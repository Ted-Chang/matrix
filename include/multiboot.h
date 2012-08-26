#ifndef __MULTIBOOT_H__
#define __MULTIBOOT_H__

/* The magic field should contain this */
#define MULTIBOOT_MAGIC			0x2BADB002
/* The bits in the required part of flags field we don't support */
#define MULTIBOOT_UNSUPPORTED		0x0000FFFC
/* Alignment of multiboot modules */
#define MULTIBOOT_MOD_ALIGN		0x00001000
/* Alignment of the multiboot info structure */
#define MULTIBOOT_INFO_ALIGN		0x00000004

/* Flags set in the `flags' member of the multiboot header */
#define MULTIBOOT_FLAG_PAGE_ALIGN	0x00000001
#define MULTIBOOT_FLAG_MEM_INFO		0x00000002
#define MULTIBOOT_FLAG_VIDEO_MODE	0x00000004
#define MULTIBOOT_FLAG_AOUT_KLUDGE	0x00010000

/* Flags set in the `flags' member of the multiboot info structure */
#define MULTIBOOT_FLAG_INFO_MEM		0x00000001
#define MULTIBOOT_FLAG_DEVICE		0x00000002
#define MULTIBOOT_FLAG_CMDLINE		0x00000004
#define MULTIBOOT_FLAG_MODS		0x00000008
/* These next two are mutually exclusive */
/* Is there a symbol table loaded? */
#define MULTIBOOT_FLAG_AOUT_SYMS	0x00000010
/* Is there an ELF section header table? */
#define MULTIBOOT_FLAG_ELF_SHDR		0x00000020
/* Is there a full memory map? */
#define MULTIBOOT_FLAG_MEM_MAP		0x00000040
/* Is there drive info? */
#define MULTIBOOT_FLAG_DRIVE_INFO	0x00000080
/* Is there a config table */
#define MULTIBOOT_FLAG_CONFIG_TABLE	0x00000100
/* Is there a boot loader name? */
#define MULTIBOOT_FLAG_LOADER_NAME	0x00000200
/* Is there a APM table */
#define MULTIBOOT_FLAG_APM_TABLE	0x00000400
/* Is there video information? */
#define MULTIBOOT_FLAG_VBE		0x00000800

typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;

/* The multiboot header */
struct multiboot_header {
	/* Must be MULTIBOOT_MAGIC */
	uint32_t magic;

	/* Feature flags */
	uint32_t flags;

	/* The above fields plus this one must equal 0 mod 2^32 */
	uint32_t checksum;

	/* These are only valid if MULTIBOOT_AOUT_KLUDGE is set */
	uint32_t header_addr;
	uint32_t load_addr;
	uint32_t load_end_addr;
	uint32_t bss_end_addr;
	uint32_t entry_addr;

	/* These are only valid if MULTIBOOT_VIDEO_MODE is set */
	uint32_t mode_type;
	uint32_t width;
	uint32_t height;
	uint32_t depth;
};

/* The symbol table for a.out */
struct multiboot_aout_symbol_table {
	uint32_t tab_size;
	uint32_t str_size;
	uint32_t addr;
	uint32_t reserved;
};

/* The section header table for ELF */
struct multiboot_elf_section_header_table {
	uint32_t num;
	uint32_t size;
	uint32_t addr;
	uint32_t shndx;
};

/* The multiboot information */
struct multiboot_info {
	/* Multiboot info version number */
	uint32_t flags;
	
	/* Available memory from BIOS */
	uint32_t mem_lower;		// memory size. Present if flags[0] is set
	uint32_t mem_upper;

	/* root partition */
	uint32_t boot_dev;		// boot device. Present if flags[1] is set
	uint32_t cmdline;		// kernel command line. Present if flags[2] is set

	/* Boot module list */
	uint32_t mods_count;		// number of modules loaded along with
					// kernel. Present if flags[3] is set
	uint32_t mods_addr;

	union {
		struct multiboot_aout_symbol_table aout_sym;
		struct multiboot_elf_section_header_table elf_sec;
	} u;

	/* Memory mapping buffer */
	uint32_t mmap_length;		// memory map. Present if flags[6] is set
	uint32_t mmap_addr;

	/* Drive info buffer */
	uint32_t drives_length;		// physical address of first drive structure
	uint32_t drives_addr;

	/* ROM configuration table */
	uint32_t config_table;		// ROM configuration table. Present if flags[8] is set
	uint32_t bootloader_name;	// Bootloader name. Present if flags[9] is set
	uint32_t apm_table;		// Advanced power management table.

	/* Video */
	uint32_t vbe_control_info;	// video bios extension (vbe).
	uint32_t vbe_mode_info;
	uint32_t vbe_mode;
	uint32_t vbe_interface_seg;
	uint32_t vbe_interface_off;
	uint32_t vbe_interface_len;
} __attribute__((packed));

struct multiboot_mmap_entry {
	uint32_t size;
	uint64_t addr;
	uint64_t len;
#define MULTIBOOT_MEMORY_AVAILABLE	1
#define MULTIBOOT_MEMORY_RESERVED	2
	uint32_t type;
} __attribute__((packed));

struct multiboot_mod_list {
	/* The memory used goes from bytes `mod_start' to `mod_end-1' inclusive */
	uint32_t mod_start;
	uint32_t mod_end;

	/* Module command line */
	uint32_t cmdline;

	/* Padding to take it to 16 bytes (must be zero) */
	uint32_t pad;
};

extern struct multiboot_info *_mbi;

#endif	/* __MULTIBOOT_H__ */
