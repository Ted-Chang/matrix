#include <types.h>
#include <stddef.h>
#include <string.h>
#include "debug.h"
#include "mm/page.h"
#include "mm/mmu.h"
#include "mm/malloc.h"
#include "proc/process.h"
#include "proc/thread.h"
#include "elf.h"

#define ELF_CLASS	ELFCLASS32
#define ELF_ENDIAN	ELFDATA2LSB
#define ELF_MACHINE	ELF_EM_386

typedef Elf32_Ehdr elf_ehdr_t;
typedef Elf32_Shdr elf_shdr_t;

struct elf_binary {
	elf_ehdr_t *ehdr;
	struct va_space *vas;
	struct vfs_node *n;
	ptr_t load_base;
	size_t load_size;
};
typedef struct elf_binary elf_binary_t;

boolean_t elf_ehdr_check(elf_ehdr_t *ehdr)
{
	boolean_t ret = FALSE;

	/* Check the magic number and version */
	if (strncmp((const char *)ehdr->e_ident, ELF_MAGIC, strlen(ELF_MAGIC)) != 0) {
		DEBUG(DL_DBG, ("Magic is not OK.\n"));
		goto out;
	} else if (ehdr->e_ident[ELF_EI_VERSION] != 1 || ehdr->e_version != 1) {
		DEBUG(DL_DBG, ("Version(%d) is not OK.\n", ehdr->e_ident[ELF_EI_VERSION]));
		goto out;
	}

	/* Check whether it matches the architecture we're running on. */
	if (ehdr->e_ident[ELF_EI_CLASS] != ELF_CLASS ||
	    ehdr->e_ident[ELF_EI_DATA] != ELF_ENDIAN ||
	    ehdr->e_machine != ELF_MACHINE) {
		DEBUG(DL_DBG, ("Machine(%d:%d:%d) is not OK.\n", ehdr->e_ident[ELF_EI_CLASS],
			       ehdr->e_ident[ELF_EI_DATA], ehdr->e_machine));
		goto out;
	}

	/* We can only load EXEC now */
	if (ehdr->e_type != ELF_ET_EXEC) {
		DEBUG(DL_DBG, ("Type(%d) is not OK.\n", ehdr->e_type));
		goto out;
	}

	ret = TRUE;

 out:
	return ret;
}

ptr_t elf_finish_binary(void *data)
{
	int i = 0;
	ptr_t entry;
	elf_binary_t *bin;
	elf_ehdr_t *ehdr;
	elf_shdr_t *shdr;
	uint32_t virt;

	bin = (elf_binary_t *)data;

	ehdr = bin->ehdr;

	for (virt = 0; virt < (ehdr->e_shentsize * ehdr->e_shnum); virt += ehdr->e_shentsize, i++) {
		shdr = (elf_shdr_t *)(((uint8_t *)ehdr) + (ehdr->e_shoff + virt));

		if (shdr->sh_addr) {
			switch (shdr->sh_type) {
			case ELF_SHT_NOBITS:
				/* Zero the .bss section */
				memset((void *)(shdr->sh_addr), 0, shdr->sh_size);
				DEBUG(DL_DBG, (".bss section zeroed.\n"));
				break;	// Break out the switch
			case ELF_SHT_PROGBITS:
			case ELF_SHT_STRTAB:
			case ELF_SHT_SYMTAB:
				/* Copy the section into memory */
				memcpy((void *)(shdr->sh_addr), (uint8_t *)ehdr + shdr->sh_offset,
				       shdr->sh_size);
				DEBUG(DL_DBG, ("section type %d copied.\n", shdr->sh_type));
				break;	// Break out the switch
			}
		}
	}

	entry = ehdr->e_entry;
	DEBUG(DL_DBG, ("entry(%p).\n", entry));
	
	kfree(bin->ehdr);
	kfree(bin);

	return entry;
}

int elf_load_binary(struct vfs_node *n, struct va_space *vas, void **datap)
{
	int rc = -1, i;
	size_t size, load_cnt;
	elf_binary_t *bin;
	ptr_t virt, base;
	elf_shdr_t *shdr;
	elf_ehdr_t *ehdr;

	/* Allocate buffer to store the binary information */
	bin = kmalloc(sizeof(elf_binary_t), 0);
	if (!bin) {
		DEBUG(DL_INF, ("kmalloc buffer for binary failed.\n"));
		rc = -1;
		goto out;
	}
	
	bin->vas = vas;
	bin->n = n;

	/* Allocate buffer to store the file content */
	bin->ehdr = kmalloc(n->length, 0);
	if (!bin->ehdr) {
		DEBUG(DL_INF, ("kmalloc buffer for file failed.\n"));
		rc = -1;
		goto out;
	}
	
	/* Read in the executive content */
	rc = vfs_read(n, 0, n->length, (uint8_t *)bin->ehdr);
	if (rc == -1 || rc < sizeof(elf_ehdr_t)) {
		DEBUG(DL_INF, ("read file failed, err(%d).\n", rc));
		rc = -1;
		goto out;
	}

	/* Check whether it is valid ELF */
	if (!elf_ehdr_check(bin->ehdr)) {
		DEBUG(DL_INF, ("invalid ELF file.\n"));
		rc = -1;
		goto out;
	}

	ehdr = bin->ehdr;
	base = ehdr->e_entry;
	
	/* Load the loadable segments from the executive to the address which was
	 * specified in the ELF. For Matrix default is 0x20000000 which was specified
	 * in the link script.
	 */
	load_cnt = 0;
	for (i = 0, virt = 0;
	     virt < (ehdr->e_shentsize * ehdr->e_shnum);
	     virt += ehdr->e_shentsize, i++) {

		/* Read a section header */
		shdr = (elf_shdr_t *)(((uint8_t *)ehdr) + (ehdr->e_shoff + virt));
		DEBUG(DL_DBG, ("i(%d), sh_addr(0x%x), sh_size(0x%x), sh_type(%d)\n",
			       i, shdr->sh_addr, shdr->sh_size, shdr->sh_type));
		
		if (shdr->sh_addr) {
			size_t map_size;
			
			/* If this is a loadable section, load it */
			if (shdr->sh_addr < base) {
				/* If this is the lowest entry point, store it */
				base = shdr->sh_addr;
			}

			/* Update the section size we may needed */
			if ((shdr->sh_addr + shdr->sh_size - base) > size) {
				/* We also store the total size */
				size = shdr->sh_addr + shdr->sh_size - base;
			}

			/* Map address space for this section, this is where codes stored */
			map_size = ROUND_UP(shdr->sh_size, PAGE_SIZE);
			rc = va_map(vas, shdr->sh_addr, map_size,
				    VA_MAP_READ|VA_MAP_WRITE|VA_MAP_FIXED, NULL);
			if (rc != 0) {
				DEBUG(DL_WRN, ("va_map failed, err(%d).\n", rc));
				goto out;
			}

			load_cnt++;
			DEBUG(DL_DBG, ("i(%d), address space mapped.\n", i));
		}
	}

	/* Check whether we actually loaded anything */
	if (!load_cnt) {
		rc = -1;
		DEBUG(DL_WRN, ("binary do not have any loadable sections.\n"));
		goto out;
	}

	bin->load_base = base;
	
	/* Save the entry point to the code segment */
	*datap = bin;
	DEBUG(DL_DBG, ("base(%p), datap (%p)\n", base, *datap));

	rc = 0;

out:
	if (rc != 0) {
		if (bin) {
			if (bin->ehdr) {
				kfree(bin->ehdr);
			}
			kfree(bin);
		}
	}
	
	return rc;
}
