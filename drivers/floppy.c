#include <types.h>
#include "matrix.h"
#include "hal.h"
#include "isr.h"
#include "floppy.h"
#include "util.h"

/* I/O Ports used by floppy disk */
#define DOR		0x3F2
#define FDC_STATUS	0x3F4
#define FDC_DATA	0x3F5
#define FDC_RATE	0x3F7
#define DMA_ADDR	0x004
#define DMA_TOP		0x081
#define DMA_COUNT	0x005
#define DMA_FLIPFLOP	0x00C
#define DMA_MODE	0x00B
#define DMA_INIT	0x00A
#define DMA_RESET_VAL	0x006

#define DMA_ADDR_MASK	0xFFFFFF	// mask to verify DMA address is 24-bits

/* Status registers returned as result of operation */
#define ST0		0x00
#define ST1		0x01
#define ST2		0x02
#define ST3		0x00
#define ST_CYL		0x03
#define ST_HEAD		0x04
#define ST_SEC		0x05
#define ST_PCN		0x01

/* Main Status Register */
#define CTL_BUSY	0x10
#define DIRECTION	0x40
#define MASTER		0x80

/* ST0 */
#define ST0_BITS_TRANS	0xD8
#define TRANS_ST0	0x00
#define ST0_BITS_SEEK	0xF8
#define SEEK_ST0	0x20

/* ST1 */
#define BAD_SECTOR	0x05
#define WRITE_PROTECT	0x02

/* ST2 */
#define BAD_CYL		0x1F

/* ST3 (not used) */
#define ST3_FAULT	0x80
#define ST3_WR_PROTECT	0x40
#define ST3_READY	0x20

/* Floppy disk controller command bytes */
#define FDC_SEEK	0x0F
#define FDC_READ	0xE6
#define FDC_WRITE	0xC5
#define FDC_SENSE	0x08
#define FDC_RECALIBRATE	0x07
#define FDC_SPECIFY	0x03
#define FDC_READ_ID	0x4A
#define FDC_FORMAT	0x4D

/* Floppy disk drive status bits */
#define FDD_CHANGED	(1 << 0)
#define FDD_SPINUP	(1 << 1)
#define FDD_SPINDN	(1 << 2)

#define IS_MOTOR_ON(f, d)	(f->dor & (1 << (d + 4)))

/* Forward declaration */
struct fdc;

/* Status for a floppy disk drive */
struct fdd {
	struct fdc *fdc;	// Controller of this drive
	const struct flpy_fmt *fmt;	// Current format of this floppy
	uint32_t number;	// Drive number per FDC
	uint32_t flags;		// Flags for this floppy disk drive
	uint32_t track;		// Current floppy track sought
	void *spin_down;
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

/* Floppy format */
struct flpy_fmt{
	uint32_t size;
	uint32_t sector_per_track;
	uint32_t heads;
	uint32_t tracks;
	uint32_t strech;
	int8_t gap3;
	int8_t rate;
	int8_t sr_hut;
	int8_t gap3fmt;
	const char *name;
};

static struct irq_hook _flpy_hook;
static volatile boolean_t _irq_signaled = FALSE;

static const struct flpy_fmt flpy_fmts[] = {
	{0, 0, 0, 0, 0, 0x00, 0x00, 0x00, 0x00, NULL},		// 0 dummy entry
	{720, 9, 2, 40, 0, 0x2A, 0x02, 0xDF, 0x50, "d360"},	// 1 360KB PC
	{2400, 15, 2, 80, 0, 0x1B, 0x00, 0xDF, 0x54, "h1200"},	// 2 1.2MB AT
	{720, 9, 1, 80, 0, 0x2A, 0x02, 0xDF, 0x50, "D360"},	// 3 360KB SS 3.5"
	{1440, 9, 2, 80, 0, 0x2A, 0x02, 0xDF, 0x50, "D720"},	// 4 720KB 3.5"
	{720, 9, 2, 40, 1, 0x23, 0x01, 0xDF, 0x50, "h360"},	// 5 360KB AT
	{1440, 9, 2, 80, 0, 0x23, 0x01, 0xDF, 0x50, "h720"},	// 6 720KB AT
	{2880, 18, 2, 80, 0, 0x1B, 0x00, 0xCF, 0x6C, "H1440"},	// 7 1.44MB 3.5"
};

static void recalibrate()
{
}

/* Turns the motor on and select drive */
static void start_motor(struct fdd *f)
{
	if (FLAG_ON(f->flags, FDD_SPINDN))
		;
	CLEAR_FLAG(f->flags, FDD_SPINDN);
	if (!IS_MOTOR_ON(f->fdc, f->number)) {
		if (!FLAG_ON(f->flags, FDD_SPINUP))
			;
		/* Select drive */
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

void flpy_reset()
{
	;
}

void flpy_seek()
{
	;
}

void init_floppy()
{
	register_interrupt_handler(IRQ6, &_flpy_hook, flpy_callback);

	flpy_reset();
}
