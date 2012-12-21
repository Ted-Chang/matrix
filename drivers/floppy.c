#include <types.h>
#include <stddef.h>
#include "matrix/matrix.h"
#include "hal/hal.h"
#include "hal/isr.h"
#include "floppy.h"
#include "util.h"
#include "debug.h"


#define FDC_PRI		0x3F0		// Base port of the primary controller
#define FDC_SEC		0x370		// Base port of the secondary controller

/* I/O Ports used by floppy disk */
#define FDC_DOR		0x002
#define FDC_MSR		0x004		// Main Status Register
#define FDC_DATA	0x005		// Data Register
#define FDC_DIR		0x007		// Digital Input Regsiter
#define FDC_CCR		0x007		// Configuration Control Register


/* DMA transfers */
#define DMA_CHANNEL	2		// Number of the DMA channel for floppy transfer
#define DMA_PAGE	0x81		// Page register for that DMA channel
#define DMA_OFFSET	0x04		// Offset register for that DMA channel
#define DMA_LENGTH	0x05		// Length register for that DMA channel
#define DMA_FLIPFLOP	0x0C
#define DMA_MODE	0x0B

#define DMA_ADDR_MASK	0xFFFFFF	// mask to verify DMA address is 24-bits


/* Floppy disk controller command bytes */
#define FDC_SEEK	0x0F	// Seek track
#define FDC_READ	0xE6	// Read data (+ MT,MFM,SK)
#define FDC_WRITE	0xC5	// Write data (+ MT,MFM)
#define FDC_SENSE	0x08	// Sense interrupt status
#define FDC_RECALIBRATE	0x07	// Recalibrate
#define FDC_SPECIFY	0x03	// Specify drive timings
#define FDC_READ_ID	0x4A	// Read sector ID (+ MFM)
#define FDC_FORMAT	0x4D	// Format track
#define FDC_VERSION	0x10	// Get FDC version

/* Floppy disk drive status bits */
#define FDD_CHANGED	(1 << 0)
#define FDD_SPINUP	(1 << 1)
#define FDD_SPINDN	(1 << 2)

#define IS_MOTOR_ON(f, d)	(f->dor & (1 << (d + 4)))

/* Miscellaneous */
#define NR_MAXDRIVES	2	// Maximum number of floppy drives

/* CHS for floppy disk addressing */
struct chs {
	uint32_t c;
	uint32_t h;
	uint32_t s;
};

/* Floppy disk controller */
struct fdc {
	uint32_t base_port;	// Base port for this controller
	uint8_t result[7];	// Stores the result bytes returned
	uint8_t result_size;	// Number of result bytes returned
	uint8_t sr0;		// Status Register 0 after a sense interrupt
	uint8_t dor;		// Reflects the Digital Output Register
	struct fdd drive[4];	// Drives connected to this controller
};

struct flpy_fmt _flpy_fmts[] = {
	{0, 0, 0, 0, 0, 0x00, 0x00, 0x00, 0x00, NULL},
	{720, 9, 2, 40, 0, 0x2A, 0x02, 0xDF, 0x50, "d360"},
};

#define MS 1000
struct drive_param _dft_drive_params[] = {
	{0, 0, 1000*MS, 3000*MS, 20*MS, 3000*MS, {7, 4, 8, 2, 1, 5, 3,10}, 0, "unknown"},
	{1, 4, 1000*MS, 3000*MS, 20*MS, 3000*MS, {1, 0, 0, 0, 0, 0, 0, 0}, 1, "5.25\" DD, 360 KiB"},
	{2, 8,  400*MS, 3000*MS, 20*MS, 3000*MS, {2, 5, 6,23,10,20,12, 0}, 2, "5.25\" HD, 1200 KiB"},
	{3, 4, 1000*MS, 3000*MS, 20*MS, 3000*MS, {4,22,21,30, 3, 0, 0, 0}, 4, "3.5\" DD, 720 KiB"},
	{4, 8,  400*MS, 3000*MS, 20*MS, 3000*MS, {7, 4,25,22,31,21,29,11}, 7, "3.5\" HD, 1440 KiB"},
	{5,15,  400*MS, 3000*MS, 20*MS, 3000*MS, {7, 8, 4,25,28,22,31,21}, 8, "3.5\" ED, 2880 KiB AMI"},
	{6,15,  400*MS, 3000*MS, 20*MS, 3000*MS, {7, 8, 4,25,28,22,31,21}, 8, "3.5\" ED, 2880 KiB"}
};

static struct irq_hook _flpy_hook;
static volatile boolean_t _irq_signaled = FALSE;
static volatile boolean_t _busy = FALSE;

static uint32_t dma_addr;	// Physical address of DMA buffer

/* Primary floppy control */
static struct fdc _primary_fdc;
static struct fdc _secondary_fdc;


static void lba2chs(uint32_t lba, struct chs *chs_ptr, uint32_t sectors_per_track)
{
}

static int dma_xfer(uint32_t physical_addr, uint32_t len, boolean_t read)
{
	uint32_t page, off;

	/* Calculate DMA page and offset */
	page = physical_addr >> 16;
	off = physical_addr & 0xFFFF;
	len -= 1;	// With DMA, if you want k bytes, you need to ask for k-1 bytes

	irq_disable();
	outportb(DMA_FLIPFLOP, DMA_CHANNEL | 4);	// Set channel mask bit
	outportb(DMA_FLIPFLOP, 0);			// Clear flip flop
	outportb(DMA_MODE, (read ? 0x48 : 0x44) + DMA_CHANNEL); // Mode
	outportb(DMA_PAGE, page);			// Page
	outportb(DMA_OFFSET, off & 0xFF);		// Offset: low bytes
	outportb(DMA_OFFSET, off >> 8);			// Offset: high bytes
	outportb(DMA_LENGTH, len & 0xFF);		// Length: low bytes
	outportb(DMA_LENGTH, len >> 8);			// Length: high bytes
	outportb(DMA_FLIPFLOP, DMA_CHANNEL);		// Clear channel mask bit
	irq_enable();
	
	return -1;
}

static int fdc_out(u_long base_port, uint8_t val)
{
	volatile uint32_t msr;
	u_long tmo;

	for (tmo = 0; tmo < 128; tmo++) {
		msr = inportb(base_port + FDC_MSR);
		if ((msr & 0xC0) == 0x80) {
			outportb(base_port + FDC_DATA, val);
			return 0;	// TODO: Replace it with the error code
		}
		inportb(0x80);	// Delay
	}

	return -1;			// TODO: Replace it with the error code
}

static int fdc_in(u_long base_port)
{
	volatile uint32_t msr;
	u_long tmo;

	for (tmo = 0; tmo < 128; tmo++) {
		msr = inportb(base_port + FDC_MSR);
		if ((msr & 0xD0) == 0xD0)
			return inportb(base_port + FDC_DATA);
		inportb(0x80);	// Delay
	}

	return -1;			// TODO: Replace it with the error code
}

static int wait_fdc(struct fdd *d)
{
	volatile boolean_t irq_timeout;

	/* Wait for the interrupt handler to signal command finished */
	// ...
	
	/* Read in command result bytes while controller is busy */
	d->fdc->result_size = 0;
	while ((d->fdc->result_size < 7) && (fdc_in(d->fdc->base_port + FDC_MSR) & 0x10))
		d->fdc->result[d->fdc->result_size++] = fdc_in(d->fdc->base_port);

	_irq_signaled = FALSE;
	if (irq_timeout) {
		DEBUG(DL_WRN, ("irq timeout, drive(%d)\n", d->number));
		return -1;	// TODO: Return an error code
	}

	return 0;		// TODO: Return an error code
}

static void fdc_reset(struct fdc *f)
{
	int i;
	
	f->dor = 0;
	DEBUG(DL_DBG, ("Reseting fdc, fdc->base_port(0x%x).\n", f->base_port));
	fdc_out(f->base_port + FDC_DOR, 0);	// Stop the motor and disable IRQ/DMA
	fdc_out(f->base_port + FDC_DOR, 0x0C);	// Re-enable IRQ/DMA and release reset
	f->dor = 0x0C;

	/* Resetting triggered 4 interrupts - handle them */
	// TODO: Initialize a timer to handle the time out stuff

	/* FDC specs say to sense interrupt status four times */
	for (i = 0; i < 4; i++) {
		/* Send the sense interrupt status command */
		fdc_out(f->base_port, FDC_SENSE);
		f->sr0 = fdc_in(f->base_port);
		f->drive[i].track = fdc_in(f->base_port);
	}

	_irq_signaled = FALSE;
}

static void specify(const struct fdd *d)
{
	outportb(d->fdc->base_port + FDC_CCR, d->fmt->rate);
	fdc_out(d->fdc->base_port, FDC_SPECIFY);
	fdc_out(d->fdc->base_port, d->fmt->sr_hut);
	fdc_out(d->fdc->base_port, d->param->hlt << 1);	// Always DMA
}

static void recalibrate(struct fdd *d)
{
	int i;
	
	for (i = 0; i < 13; i++) {
		fdc_out(d->fdc->base_port, FDC_RECALIBRATE);
		fdc_out(d->fdc->base_port, d->number);
		wait_fdc(d);
		/* Send a `sense interrupt status' command */
		fdc_out(d->fdc->base_port, FDC_SENSE);
		d->fdc->sr0 = fdc_in(d->fdc->base_port);
		d->track = fdc_in(d->fdc->base_port);

		if (!(d->fdc->sr0 & 0x10)) break;	// Exit if unit check is not set
	}

	DEBUG(DL_DBG, ("drive(%d), sr0(0x%x), track(%d)\n",
		       d->number, d->fdc->sr0, d->track));
}

static int setup_drive(struct fdc *f, uint8_t n, uint32_t cmos_type)
{
	f->drive[n].fdc = f;
	f->drive[n].param = &_dft_drive_params[0];
	/* No need to initialize format */
	f->drive[n].number = n;
	f->drive[n].flags = 0;
	f->drive[n].track = 0;
	f->drive[n].spin_down = NULL;
	
	if ((cmos_type > 0) && (cmos_type <= 6)) {
		DEBUG(DL_DBG, ("Floppy drive %d set to %s\n",
			       n, _dft_drive_params[cmos_type].name));
		f->drive[n].param = &_dft_drive_params[cmos_type];
		f->drive[n].flags = FDD_CHANGED;
	}

	return 0;
}

/* Turns the motor on and select drive */
static void start_motor(struct fdd *f)
{
	if (FLAG_ON(f->flags, FDD_SPINDN))
		/* Cancel the spindown timer */
		;
	CLEAR_FLAG(f->flags, FDD_SPINDN);
	if (!IS_MOTOR_ON(f->fdc, f->number)) {
		if (!FLAG_ON(f->flags, FDD_SPINUP))
			/* TODO: Register motor spin callback */
			;
		f->fdc->dor |= (1 << (f->number + 4));
		/* Select drive */
		f->fdc->dor = (f->fdc->dor & 0xFC) | f->number;
		fdc_out(f->fdc->base_port + FDC_DOR, f->fdc->dor);
	}
}

/* Schedule the motor off */
static void stop_motor(struct fdd *f)
{
	if (IS_MOTOR_ON(f->fdc, f->number)) {
		if (FLAG_ON(f->flags, FDD_SPINDN))
			return;
		SET_FLAG(f->flags, FDD_SPINDN);
	}
}

static void flpy_callback(struct registers *regs)
{
	_irq_signaled = TRUE;
	kprintf("flpy_callback: interrupt received!\n");
}

int flpy_seek(struct fdd *d, uint32_t track)
{
	if (d->track == track) return 0;
	fdc_out(d->fdc->base_port, FDC_SEEK);
	fdc_out(d->fdc->base_port, d->number);
	fdc_out(d->fdc->base_port, track);

	if (wait_fdc(d)) return -1;	// Timeout

	/* Send a `sense interrupt status' command */
	fdc_out(d->fdc->base_port, FDC_SENSE);
	d->fdc->sr0 = fdc_in(d->fdc->base_port);
	d->track = fdc_in(d->fdc->base_port);

	/* Check that seek worked */
	if ((d->fdc->sr0 != 0x20 + d->number) || (d->track != track)) {
		DEBUG(DL_ERR, ("error on drive(%d)\n"
			       "* sr0(0x%x), track(%d), expected(%d)\n",
			       d->number, d->fdc->sr0, d->track, track));
		return -1;	// TODO: return an error code
	}

	return 0;
}

static int fdc_xfer(struct fdd *d, const struct chs *chs_p, uint32_t dma_addr,
		    uint32_t nr_sectors, uint32_t op)
{
	return -1;
}

int flpy_read(struct fdd *d, uint32_t lba, uint8_t *buf, uint32_t nr_sectors)
{
	int rc = 0;
	uint8_t retries;
	struct chs f_addr;

	if (d->param->cmos_type == 0) return -1;

	while (_busy) ;	/* Wait while the floppy driver is already busy. BUSY WAIT! */
	_busy = TRUE;

	start_motor(d);

	specify(d);

	/* Only retry for 3 times */
	for (retries = 0; retries < 3; retries++) {
		/* Move head to right track */
		if (flpy_seek(d, f_addr.c) == 0) {
			/* If changeline is active, no disk is in drive */
			if (inportb(d->fdc->base_port + FDC_DIR) & 0x80) {
				DEBUG(DL_ERR, ("no disk in drive %d\n",
					       d->number));
				rc = -1;
				goto errorout;
			}
			rc = fdc_xfer(d, &f_addr, dma_addr, nr_sectors, FDC_READ);
			if (rc == 0) break;
		} else
			rc = -1;
		if (retries < 2) recalibrate(d);
	}

	/* Copy data from the DMA buffer into the caller's buffer */
	if ((rc == 0) && buf)
		;

 errorout:
	stop_motor(d);
	_busy = FALSE;

	return rc;
}

int flpy_write(struct fdd *d, uint32_t lba, const uint8_t buf, uint32_t nr_sectors)
{
	int rc = 0;
	uint8_t retries;
	struct chs f_addr;

	if (d->param->cmos_type == 0) return -1;
	
	while (_busy) ;	// The BUSY WAIT!
	_busy = TRUE;

	start_motor(d);

	specify(d);
	
	for (retries = 0; retries < 3; retries++) {
		/* Move head to right track */
		if (flpy_seek(d, f_addr.c) == 0) {
			/* If changeline is active, no disk is in drive */
			if (inportb(d->fdc->base_port + FDC_DIR) & 0x80) {
				DEBUG(DL_ERR, ("no disk in drive %d\n",
					       d->number));
				rc = -1;
				break;
			}
			rc = fdc_xfer(d, &f_addr, dma_addr, nr_sectors, FDC_WRITE);
			if (rc == 0) break;
		} else
			rc = -1;
		if (retries < 2 ) recalibrate(d);
	}

	stop_motor(d);
	_busy = FALSE;

	return rc;
}

int floppy_init(void)
{
	int res, i;
	u_long cmos_drive0, cmos_drive1;

	/* Setup the interrupt handler */
	register_irq_handler(IRQ6, &_flpy_hook, flpy_callback);

	/* Reset primary controller */
	_primary_fdc.base_port = FDC_PRI;
	fdc_reset(&_primary_fdc);
	
	/* Get the FDC version */
	fdc_out(_primary_fdc.base_port, FDC_VERSION);
	res = fdc_in(_primary_fdc.base_port);
	DEBUG(DL_DBG, ("FDC version(0x%x)\n", res));

	switch (res) {
	case 0x80: DEBUG(DL_DBG, ("NEC765 FDC found on base port 0x%x\n",
				  _primary_fdc.base_port));
		break;
	case 0x90: DEBUG(DL_DBG, ("Enhanced FDC found on base port 0x%x\n",
				  _primary_fdc.base_port));
		break;
	default: DEBUG(DL_DBG, ("FDC not found on base port 0x%x\n",
				_primary_fdc.base_port));
	}

	/* Read floppy drive type from CMOS memory (up to two drives). */
	fdc_out(0x70, 0x10);
	res = fdc_in(0x71);
	cmos_drive0 = res >> 4;
	cmos_drive1 = res & 0x0F;

	/* Setup the two floppy drives */
	setup_drive(&_primary_fdc, 0, cmos_drive0);
	setup_drive(&_primary_fdc, 0, cmos_drive1);

	for (res = 0, i = 0; i < NR_MAXDRIVES; i++) {
		if (_primary_fdc.drive[i].param->cmos_type)
			;	// Setup callback
	}

	DEBUG(DL_DBG, ("module flpy initialize successfully.\n"));
	
	return 0;
}
