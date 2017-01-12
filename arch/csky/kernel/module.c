#include <linux/moduleloader.h>
#include <linux/elf.h>
#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <asm/pgtable.h>

#define DEBUGP(fmt...)

void *module_alloc(unsigned long size)
{
	if (size == 0)
		return NULL;

	return vmalloc(size);
}

/*
 * Free memory returned from module_alloc
 */
void module_free(struct module *mod, void *module_region)
{
	vfree(module_region);
	/* FIXME: If module_region == mod->init_region, trim exception
	   table entries. */
}

int apply_relocate(Elf_Shdr *sechdrs, const char *strtab, unsigned int symindex,
                unsigned int relsec, struct module *me)
{
	unsigned int i;
	Elf32_Rel *rel = (void *)sechdrs[relsec].sh_addr;
	Elf32_Sym *sym;
	uint32_t *location;
	short * temp;

	DEBUGP("Applying relocate section %u to %u\n", relsec,
		sechdrs[relsec].sh_info);
	for (i = 0; i < sechdrs[relsec].sh_size / sizeof(*rel); i++) {
		/* This is where to make the change */
		location = (void *)sechdrs[sechdrs[relsec].sh_info].sh_addr
		        + rel[i].r_offset;
		/* This is the symbol it is referring to.  Note that all
		   undefined symbols have been resolved.  */
		sym = (Elf32_Sym *)sechdrs[symindex].sh_addr
		        + ELF32_R_SYM(rel[i].r_info);
		switch (ELF32_R_TYPE(rel[i].r_info)) {
		case R_CSKY_NONE:
		case R_CSKY_PCRELJSR_IMM11BY2:
		case R_CSKY_PCRELJSR_IMM26BY2:
			/* ignore */
			break;
		case R_CSKY_32:
			/* We add the value into the location given */
			*location += sym->st_value;
			break;
		case R_CSKY_PC32:
			/* Add the value, subtract its postition */
			*location += sym->st_value - (uint32_t)location;
			break;
		case R_CSKY_ADDR_HI16:
			temp = ((short  *)location) + 1;
			*temp = (short)((sym->st_value) >> 16);
			break;
		case R_CSKY_ADDR_LO16:
			temp = ((short  *)location) + 1;
			*temp = (short)((sym->st_value) & 0xffff);
			break;
		default:
			printk(KERN_ERR "module %s: Unknown relocation: %u\n",
			       me->name, ELF32_R_TYPE(rel[i].r_info));
			return -ENOEXEC;
		}
	}
	return 0;
}

int apply_relocate_add(Elf32_Shdr *sechdrs, const char *strtab,
               unsigned int symindex, unsigned int relsec, struct module *me)
{
	unsigned int i;
	Elf32_Rela *rel = (void *)sechdrs[relsec].sh_addr;
	Elf32_Sym *sym;
	uint32_t *location;
	short * temp;
#ifdef __CSKYABIV2__
	uint32_t *nop_location;
#endif

	DEBUGP("Applying relocate_add section %u to %u\n", relsec,
	       sechdrs[relsec].sh_info);
	for (i = 0; i < sechdrs[relsec].sh_size / sizeof(*rel); i++) {
		/* This is where to make the change */
		location = (void *)sechdrs[sechdrs[relsec].sh_info].sh_addr
		    + rel[i].r_offset;
		/* This is the symbol it is referring to.  Note that all
		   undefined symbols have been resolved.  */
		sym = (Elf32_Sym *)sechdrs[symindex].sh_addr
		    + ELF32_R_SYM(rel[i].r_info);

		switch (ELF32_R_TYPE(rel[i].r_info)) {
		case R_CSKY_32:
			/* We add the value into the location given */
			*location = rel[i].r_addend + sym->st_value;
			break;
		case R_CSKY_PC32:
			/* Add the value, subtract its postition */
			*location = rel[i].r_addend + sym->st_value
			     - (uint32_t)location;
			break;
		case R_CSKY_PCRELJSR_IMM11BY2:
			break;
		case R_CSKY_PCRELJSR_IMM26BY2:
		/* Relocate jsri */
#ifdef __CSKYABIV2__
			*location = (((*location) & 0xffffff9f) | 0x00000019);
			if (*(location + 1) == 0x4820c400)
			{
				*(location + 1) = 0x0000e8f9;
			}
			else
			{
				/* If the constant pool is between jsri and nop, then we should do this.
				   The "else{}" can be deleted after the toolchain's optimizion. */
				nop_location = (uint32_t *)((char*)location + 4 + ((*((short*)(location + 1)) & 0x03ff) << 1));
				*nop_location = 0x0000e8f9;
			}
#endif
			break;
		case R_CSKY_ADDR_HI16:
			temp = ((short  *)location) + 1;
			*temp = (short)((rel[i].r_addend + sym->st_value) >> 16);
			 break;
		case R_CSKY_ADDR_LO16:
			temp = ((short  *)location) + 1;
			*temp = (short)((rel[i].r_addend + sym->st_value) & 0xffff);
			break;
		default:
			printk(KERN_ERR "module %s: Unknown relocation: %u\n",
			       me->name, ELF32_R_TYPE(rel[i].r_info));
			return -ENOEXEC;
		}
	}
	return 0;
}

