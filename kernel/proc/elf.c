#include <types.h>
#include <stddef.h>
#include <string.h>
#include "matrix/debug.h"
#include "mm/page.h"
#include "mm/mmu.h"
#include "proc/process.h"
#include "elf.h"

#define ELF_CLASS	ELFCLASS32
#define ELF_ENDIAN	ELFDATA2LSB
#define ELF_MACHINE	ELF_EM_386

typedef Elf32_Ehdr elf_ehdr_t;
typedef Elf32_Shdr elf_shdr_t;

boolean_t elf_ehdr_check(elf_ehdr_t *ehdr)
{
	/* Check the magic number and version */
	if (strncmp((const char *)ehdr->e_ident, ELF_MAGIC, strlen(ELF_MAGIC)) != 0) {
		return FALSE;
	} else if (ehdr->e_ident[ELF_EI_VERSION] != 1 || ehdr->e_version != 1) {
		return FALSE;
	}

	/* Check whether it matches the architecture we're running on. */
	if (ehdr->e_ident[ELF_EI_CLASS] != ELF_CLASS ||
	    ehdr->e_ident[ELF_EI_DATA] != ELF_ENDIAN ||
	    ehdr->e_machine != ELF_MACHINE) {
		return FALSE;
	}

	return TRUE;
}

int elf_load_sections(struct thread *t, elf_ehdr_t *ehdr)
{
	int rc = -1;
	struct page *page;
	uint32_t virt;
	elf_shdr_t *shdr;
	uint32_t i = 0;

	DEBUG(DL_DBG, ("elf_load_sections: e_shnum(%d), e_shentsize(%d), e_shoff(%d)\n",
		       ehdr->e_shnum, ehdr->e_shentsize, ehdr->e_shoff));

	/* Load all the loadable sections */
	for (virt = 0; virt < (ehdr->e_shentsize * ehdr->e_shnum); virt += ehdr->e_shentsize, i++) {

		/* Read a section header */
		shdr = (elf_shdr_t *)(((uint8_t *)ehdr) + (ehdr->e_shoff + virt));
		DEBUG(DL_DBG, ("elf_load_sections: i(%d), sh_addr(0x%x), sh_size(0x%x), sh_type(%d)\n",
			       i, shdr->sh_addr, shdr->sh_size, shdr->sh_type));
		
		if (shdr->sh_addr) {
			
			/* If this is a loadable section, load it */
			if (shdr->sh_addr < t->entry) {
				/* If this is the lowest entry point, store it */
				t->entry = shdr->sh_addr;
			}

			/* Update the section size we may needed */
			/* if ((shdr->sh_addr + shdr->sh_size - arch->entry) > arch->size) { */
			/* 	/\* We also store the total size *\/ */
			/* 	arch->size = shdr->sh_addr + shdr->sh_size - arch->entry; */
			/* } */

			/* Map memory space for this section, this is where codes stored */
			for (virt = 0; virt < (shdr->sh_size + 0x2000); virt += PAGE_SIZE) {
				page = mmu_get_page(CURR_PROC->mmu_ctx, shdr->sh_addr + virt, TRUE, 0);
				if (!page) {
					DEBUG(DL_ERR, ("elf_load_section: mmu_get_page failed.\n"));
					goto out;
				}
			
				page_alloc(page, FALSE, TRUE);
			}

			DEBUG(DL_DBG, ("elf_load_sections: i(%d), memory space mapped.\n", i));
			
			switch (shdr->sh_type) {
			case ELF_SHT_NOBITS:
				/* Zero the .bss section */
				memset((void *)(shdr->sh_addr), 0, shdr->sh_size);
				break;	// Break out the switch
			case ELF_SHT_PROGBITS:
			case ELF_SHT_STRTAB:
			case ELF_SHT_SYMTAB:
				/* Copy the section into memory */
				memcpy((void *)(shdr->sh_addr), (uint8_t *)ehdr + shdr->sh_offset,
				       shdr->sh_size);
				break;	// Break out the switch
			}
		}
	}

	rc = 0;
	
out:
	return rc;
}

