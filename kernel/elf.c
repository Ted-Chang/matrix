#include <types.h>
#include <stddef.h>
#include <string.h>
#include "matrix/debug.h"
#include "elf.h"
#include "mm/page.h"
#include "mm/mmu.h"
#include "proc/process.h"

#define ELF_CLASS	ELFCLASS32
#define ELF_ENDIAN	ELFDATA2LSB
#define ELF_MACHINE	ELF_EM_386

typedef Elf32_Ehdr elf_ehdr_t;
typedef Elf32_Shdr elf_shdr_t;

static boolean_t elf_ehdr_check(elf_ehdr_t *ehdr)
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

static int elf_load_sections(struct arch_process *arch, elf_ehdr_t *ehdr)
{
	int rc = -1;
	struct page *page;
	uint32_t virt;
	elf_shdr_t *shdr;

	/* Load all the loadable sections */
	for (virt = 0; virt < (ehdr->e_shentsize * ehdr->e_shnum); virt += ehdr->e_shentsize) {

		/* Read a section header */
		shdr = (elf_shdr_t *)(((uint8_t *)ehdr) + (ehdr->e_shoff + virt));
		if (shdr->sh_addr) {
			
			/* If this is a loadable section, load it */
			if (shdr->sh_addr < arch->entry) {
				/* If this is the lowest entry point, store it */
				arch->entry = shdr->sh_addr;
			}

			if ((shdr->sh_addr + shdr->sh_size - arch->entry) > arch->size) {
				/* We also store the total size */
				arch->size = shdr->sh_addr + shdr->sh_size - arch->entry;
			}

			for (virt = 0; virt < (shdr->sh_size + 0x2000); virt += PAGE_SIZE) {
				page = mmu_get_page(_current_mmu_ctx, virt, TRUE, 0);
				if (!page) {
					DEBUG(DL_ERR, ("elf_load_section: mmu_get_page failed.\n"));
					goto out;
				}
			
				page_alloc(page, FALSE, TRUE);
			}

			switch (shdr->sh_type) {
			case ELF_SHT_NOBITS:
				/* Zero the .bss section */
				memset((void *)(shdr->sh_addr), 0, shdr->sh_size);
				break;	// Break out the switch
			case ELF_SHT_PROGBITS:
			case ELF_SHT_STRTAB:
			case ELF_SHT_SYMTAB:
				/* Copy the section into memory */
				memcpy((void *)(shdr->sh_addr), ehdr + shdr->sh_offset,
				       shdr->sh_size);
				break;	// Break out the switch
			}
		}
	}

	rc = 0;
	
out:
	return rc;
}

int exec(char *path, int argc, char **argv)
{
	int rc = -1;
	struct vfs_node *n;
	elf_ehdr_t *ehdr;
	uint32_t virt, entry;
	struct page *page;

	n = vfs_lookup(path, 0);
	if (!n) {
		return 0;
	}

	/* Map some pages to the address for this process */
	for (virt = 0x30000000;	virt < (0x30000000 + n->length); virt += PAGE_SIZE) {
		page = mmu_get_page(_current_mmu_ctx, virt, TRUE, 0);
		if (!page) {
			DEBUG(DL_ERR, ("exec: mmu_get_page failed, virt(0x%x).\n", virt));
			rc = -1;
			goto out;
		}
		
		page_alloc(page, FALSE, TRUE);
	}

	ehdr = (elf_ehdr_t *)0x30000000;

	/* Read in the executive content */
	rc = vfs_read(n, 0, n->length, (uint8_t *)ehdr);
	if (rc == -1 || rc < sizeof(elf_ehdr_t)) {
		rc = -1;
		goto out;
	}

	if (!elf_ehdr_check(ehdr)) {
		rc = -1;
		goto out;
	}

	/* Load the loadable segments from the executive */
	rc = elf_load_sections(&(CURR_PROC->arch), ehdr);
	if (rc != 0) {
		goto out;
	}

	/* Save the entry point to the code segment */
	entry = ehdr->e_entry;

	/* Free the memory we used for the ELF headers and files */
	for (virt = 0x30000000; virt < (0x30000000 + n->length); virt += PAGE_SIZE) {
		page = mmu_get_page(_current_mmu_ctx, virt, FALSE, 0);
		ASSERT(page != NULL);
		page_free(page);
	}

	/* Map some pages to the user stack */
	for (virt = USTACK_BOTTOM; virt < (USTACK_BOTTOM + USTACK_SIZE); virt += PAGE_SIZE) {
		page = mmu_get_page(_current_mmu_ctx, virt, FALSE, 0);
		if (!page) {
			DEBUG(DL_ERR, ("exec: mmu_get_page failed, virt(0x%x)\n", virt));
			rc = -1;
			goto out;
		}
		page_alloc(page, FALSE, TRUE);
	}

	CURR_PROC->arch.ustack = (USTACK_BOTTOM + USTACK_SIZE);

	/* Jump to user mode */

out:
	return -1;
}
