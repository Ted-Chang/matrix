#include <types.h>
#include <stddef.h>
#include <string.h>
#include "matrix/debug.h"
#include "mm/page.h"
#include "mm/mmu.h"
#include "proc/process.h"
#include "proc/thread.h"
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

int elf_load_sections(struct mmu_ctx *mmu, elf_ehdr_t *ehdr)
{
	int rc = -1;
	struct page *page;
	uint32_t virt, size, i = 0, entry;
	elf_shdr_t *shdr;

	DEBUG(DL_DBG, ("e_shnum(%d), e_shentsize(%d), e_shoff(%d)\n",
		       ehdr->e_shnum, ehdr->e_shentsize, ehdr->e_shoff));

	entry = ehdr->e_entry;
	size = 0;
	
	/* Load all the loadable sections */
	for (virt = 0; virt < (ehdr->e_shentsize * ehdr->e_shnum); virt += ehdr->e_shentsize, i++) {

		/* Read a section header */
		shdr = (elf_shdr_t *)(((uint8_t *)ehdr) + (ehdr->e_shoff + virt));
		DEBUG(DL_DBG, ("i(%d), sh_addr(0x%x), sh_size(0x%x), sh_type(%d)\n",
			       i, shdr->sh_addr, shdr->sh_size, shdr->sh_type));
		
		if (shdr->sh_addr) {
			
			/* If this is a loadable section, load it */
			if (shdr->sh_addr < entry) {
				/* If this is the lowest entry point, store it */
				entry = shdr->sh_addr;
			}

			/* Update the section size we may needed */
			if ((shdr->sh_addr + shdr->sh_size - entry) > size) {
				/* We also store the total size */
				size = shdr->sh_addr + shdr->sh_size - entry;
			}

			/* Map memory space for this section, this is where codes stored */
			for (virt = 0; virt < (shdr->sh_size + 0x2000); virt += PAGE_SIZE) {
				page = mmu_get_page(mmu, shdr->sh_addr + virt, TRUE, 0);
				if (!page) {
					DEBUG(DL_ERR, ("mmu_get_page failed.\n"));
					goto out;
				}
			
				page_alloc(page, FALSE, TRUE);
			}

			DEBUG(DL_DBG, ("i(%d), memory space mapped.\n", i));
			
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

int elf_load_binary(const char *path, struct mmu_ctx *mmu, void **entry)
{
	int rc = -1;
	struct vfs_node *n;
	elf_ehdr_t *ehdr;
	uint32_t virt, temp;
	struct page *page;
	boolean_t is_elf;

	/* Lookup the file from the file system */
	n = vfs_lookup(path, VFS_FILE);
	if (!n) {
		DEBUG(DL_DBG, ("file(%s) not found.\n", path));
		return -1;
	}

	/* Temporarily use address of user stack to load the ELF file, It will be
	 * unmapped after we loaded the code section in.
	 */
	temp = USTACK_BOTTOM;
	
	/* Map some pages to the address from mmu context of this process, the mapped
	 * memory page is used for storing the ELF file content
	 */
	for (virt = temp; virt < (temp + n->length); virt += PAGE_SIZE) {
		page = mmu_get_page(CURR_PROC->mmu_ctx, virt, TRUE, 0);
		if (!page) {
			DEBUG(DL_ERR, ("mmu_get_page failed, virt(0x%x).\n", virt));
			rc = -1;
			goto out;
		}
		
		page_alloc(page, FALSE, TRUE);
	}

	ehdr = (elf_ehdr_t *)temp;

	/* Read in the executive content */
	rc = vfs_read(n, 0, n->length, (uint8_t *)ehdr);
	if (rc == -1 || rc < sizeof(elf_ehdr_t)) {
		DEBUG(DL_INF, ("read file(%s) failed.\n", path));
		rc = -1;
		goto out;
	}

	/* Check whether it is valid ELF */
	is_elf = elf_ehdr_check(ehdr);
	if (!is_elf) {
		DEBUG(DL_INF, ("invalid ELF, file(%s).\n", path));
		rc = -1;
		goto out;
	}

	/* Load the loadable segments from the executive to the address which was
	 * specified in the ELF and for Matrix. default is 0x40000000 which was
	 * specified in the link script.
	 */
	rc = elf_load_sections(mmu, ehdr);
	if (rc != 0) {
		DEBUG(DL_WRN, ("load sections failed, file(%s).\n", path));
		goto out;
	}

	/* Save the entry point to the code segment */
	*entry = (void *)ehdr->e_entry;
	DEBUG(DL_DBG, ("entry(%p)\n", *entry));

	/* Free the memory we used for temporarily store the ELF files */
	for (virt = temp; virt < (temp + n->length); virt += PAGE_SIZE) {
		page = mmu_get_page(mmu, virt, FALSE, 0);
		ASSERT(page != NULL);
		page_free(page);
	}

	/* Map some pages for the user mode stack from current process' mmu context */
	for (virt = USTACK_BOTTOM; virt <= (USTACK_BOTTOM + USTACK_SIZE); virt += PAGE_SIZE) {
		page = mmu_get_page(mmu, virt, TRUE, 0);
		if (!page) {
			DEBUG(DL_ERR, ("mmu_get_page failed, virt(0x%x)\n", virt));
			rc = -1;
			goto out;
		}
		page_alloc(page, FALSE, TRUE);
	}

	rc = 0;

out:
	return rc;
}
