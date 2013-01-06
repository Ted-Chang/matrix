#ifndef __FLOPPY_H__
#define __FLOPPY_H__

struct fdc;

/* Format of a floppy disk */
struct flpy_fmt {
	uint32_t size;		// Total number of sectors on disk
	uint32_t sectors_per_track; // Number of sectors per track
	uint32_t heads;		// Number of heads (sides)
	uint32_t tracks;	// Number of tracks
	uint32_t strech;	// 1 if tracks are to be doubled
	uint8_t gap3;		// GAP3 to use for reading/writing
	uint8_t rate;		// Sector size and data transfer rate
	uint8_t sr_hut;		// Step Rate and Head Unload Time
	uint8_t gap3fmt;	// GAP3 to use for formating
	const char *name;	// Disk format name
};

/* Parameters used to handle a floppy drive */
struct drive_param {
	uint32_t cmos_type;	// Drive type read form CMOS
	uint32_t hlt;		// Head Load Time
	uint32_t spin_up;	// Time needed for spinning up in micro seconds
	uint32_t spin_down;	// Time needed for spinning down in micro seconds
	uint32_t sel_delay;	// Select delay in micro seconds
	uint32_t int_timeout;	// Interrupt timeout in micro seconds
	uint32_t detect[8];	// Autodetect formats
	uint32_t native_fmt;	// Native disk format for drive
	const char *name;	// Name displayed for the drive type
};

/* Status for a floppy disk drive */
struct fdd {
	struct fdc *fdc;	// Controller of this drive
	const struct drive_param *param; // Parameter for this drive
	const struct flpy_fmt *fmt; // Current format of the floppy
	uint32_t number;	// Drive number per FDC
	uint32_t flags;		// Flags for this floppy disk drive
	uint32_t track;		// Current floppy track sought
	void *spin_down;	// Handler for motor spindown
};


extern int floppy_init(void);

#endif	/* __FLOPPY_H__ */
