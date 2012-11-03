#ifndef __ELF_H__
#define __ELF_H__

#ifdef __KERNEL__
#include <types.h>
#else
#include <stdint.h>
#endif	/* __KERNEL__ */

/**
 * Basic 32 bit ELF Data Types
 */
typedef uint32_t Elf32_Word;
typedef uint32_t Elf32_Addr;
typedef uint32_t Elf32_Off;
typedef int32_t Elf32_Sword;
typedef uint16_t Elf32_Half;

/**
 * ELF Header
 */
typedef struct {
	unsigned char e_ident[16];	// ELF identification
	Elf32_Half e_type;		// Object file type
	Elf32_Half e_machine;		// Machine type
	Elf32_Word e_version;		// Object file version
	Elf32_Addr e_entry;		// Entry point address
	Elf32_Off e_phoff;		// Program header offset
	Elf32_Off e_shoff;		// Section header offset
	Elf32_Word e_flags;		// Processor specific flags
	Elf32_Half e_ehsize;		// ELF header size
	Elf32_Half e_phentsize;		// Size of program header entry
	Elf32_Half e_phnum;		// Number of program header entries
	Elf32_Half e_shentsize;		// Size of section header entry
	Elf32_Half e_shnum;		// Number of program header entries
	Elf32_Half e_shstrndx;		// Section name string table index
} Elf32_Ehdr;

/**
 * ELF Magic Signature
 */
#define ELF_MAGIC	"\177ELF"
#define ELF_MAG0	0x7F
#define ELF_MAG1	'E'
#define ELF_MAG2	'L'
#define ELF_MAG3	'F'

/**
 * Identification array indices
 */
#define ELF_EI_CLASS	4		// Class
#define ELF_EI_DATA	5		// Data encoding
#define ELF_EI_VERSION	6		// Version
#define ELF_EI_OSABI	7		// OS/ABI

/**
 * e_type
 */
#define ELF_ET_NONE	0		// No file type
#define ELF_ET_REL	1		// Relocatable file
#define ELF_ET_EXEC	2		// Executable file
#define ELF_ET_DYN	3		// Shared object file
#define ELF_ET_CORE	4		// Core file
#define ELF_ET_LOPROC	0xFF0		// [Processor specific]
#define ELF_ET_HIPROC	0xFFF		// [Processor specific]

/**
 * ELF classes
 */
#define ELFCLASS32	1		// 32-bit
#define ELFCLASS64	2		// 64-bit

/**
 * Endian types
 */
#define ELFDATA2LSB	1		// Little-endian
#define ELFDATA2MSB	2		// Big-endian

/**
 * Machine types
 */
#define ELF_EM_NONE	0		// No machine
#define ELF_EM_M32	1		// AT&T WE 32100
#define ELF_EM_SPARC	2		// SUN Sparc
#define ELF_EM_386	3		// Intel 80386
#define ELF_EM_68K	4		// Motorola m68k family
#define ELF_EM_88K	5		// Motorola m88k family
#define ELF_EM_860	7		// Intel 80860
#define ELF_EM_MIPS	8		// MIPS R3000 big-endian

#define ELF_EV_NONE	0
#define ELF_EV_CURRENT	1

/* Program Header */
typedef struct {
	Elf32_Word p_type;
	Elf32_Off p_offset;
	Elf32_Addr p_vaddr;
	Elf32_Addr p_paddr;
	Elf32_Word p_filesz;
	Elf32_Word p_flags;
	Elf32_Word p_align;
} Elf32_Phdr;

/**
 * p_type
 */
#define ELF_PT_NULL	0		// Unused
#define ELF_PT_LOAD	1		// Loadable segment
#define ELF_PT_DYNAMIC	2		// Dynamic link information
#define ELF_PT_INTERP	3		// Interpreter
#define ELF_PT_NOTE	4		// Auxillary information
#define ELF_PT_SHLIB	5		// Reserved
#define ELF_PT_PHDR	6		// Back reference to the header table itself
#define ELF_PT_LOPROC	0x70000000
#define ELF_PT_HIPROC	0x7FFFFFFF

/* Section Header */
typedef struct {
	Elf32_Word sh_name;
	Elf32_Word sh_type;
	Elf32_Word sh_flags;
	Elf32_Addr sh_addr;
	Elf32_Off sh_offset;
	Elf32_Word sh_size;
	Elf32_Word sh_link;
	Elf32_Word sh_info;
	Elf32_Word sh_addralign;
	Elf32_Word sh_entsize;
} Elf32_Shdr;

/**
 * sh_type
 */
#define ELF_SHT_NONE	0
#define ELF_SHT_PROGBITS 1
#define ELF_SHT_SYMTAB	2
#define ELF_SHT_STRTAB	3
#define ELF_SHT_NOBITS	8


typedef Elf32_Ehdr elf_ehdr_t;
typedef Elf32_Shdr elf_shdr_t;

extern boolean_t elf_ehdr_check(elf_ehdr_t *ehdr);
extern int elf_load_sections(struct thread *t, elf_ehdr_t *ehdr);

#endif	/* __ELF_H__ */
