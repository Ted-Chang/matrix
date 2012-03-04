#ifndef __MULTIBOOT_H__
#define __MULTIBOOT_H__

#define MULTIBOOT_FLAG_MEM	0x0001
#define MULTIBOOT_FLAG_DEVICE	0x0002
#define MULTIBOOT_FLAG_CMDLINE	0x0004
#define MULTIBOOT_FLAG_MODS	0x0008
#define MULTIBOOT_FLAG_AOUT	0x0010
#define MULTIBOOT_FLAG_ELF	0x0020
#define MULTIBOOT_FLAG_MMAP	0x0040
#define MULTIBOOT_FLAG_CONFIG	0x0080
#define MULTIBOOT_FLAG_LOADER	0x0100
#define MULTIBOOT_FLAG_APM	0x0200
#define MULTIBOOT_FLAG_VBE	0x0400

struct multiboot {
	uint32_t flags;
	uint32_t mem_lower;		// memory size. Present if flags[0] is set
	uint32_t mem_upper;
	uint32_t boot_dev;		// boot device. Present if flags[1] is set
	uint32_t cmdline;		// kernel command line. Present if flags[2] is set
	uint32_t mods_count;		// number of modules loaded along with
					// kernel. Present if flags[3] is set
	uint32_t mods_addr;
	uint32_t num;
	uint32_t size;
	uint32_t addr;
	uint32_t shndx;
	uint32_t mmap_length;		// memory map. Present if flags[6] is set
	uint32_t mmap_addr;
	uint32_t drives_length;		// physical address of first drive structure
	uint32_t drives_addr;
	uint32_t config_table;		// ROM configuration table. Present if flags[8] is set
	uint32_t bootloader_name;	// Bootloader name. Present if flags[9] is set
	uint32_t apm_table;		// Advanced power management table.
	uint32_t vbe_control_info;	// video bios extension (vbe).
	uint32_t vbe_mode_info;
	uint32_t vbe_mode;
	uint32_t vbe_interface_seg;
	uint32_t vbe_interface_off;
	uint32_t vbe_interface_len;
} __attribute__((packed));

#endif	/* __MULTIBOOT_H__ */
