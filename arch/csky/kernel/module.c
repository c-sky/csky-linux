// SPDX-License-Identifier: GPL-2.0
// Copyright (C) 2018 Hangzhou C-SKY Microsystems co.,ltd.
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

#define IS_BSR32(hi16, lo16)        (((hi16) & 0xFC00) == 0xE000)
#define IS_JSRI32(hi16, lo16)       ((hi16) == 0xEAE0)
#define CHANGE_JSRI_TO_LRW(addr)    *(uint16_t *)(addr) = (*(uint16_t *)(addr) & 0xFF9F) | 0x0019; \
							  *((uint16_t *)(addr) + 1) = *((uint16_t *)(addr) + 1) & 0xFFFF
#define SET_JSR32_R25(addr)         *(uint16_t *)(addr) = 0xE8F9; \
							  *((uint16_t *)(addr) + 1) = 0x0000;

int apply_relocate(Elf_Shdr *sechdrs, const char *strtab, unsigned int symindex,
		unsigned int relsec, struct module *me)
{
	unsigned int i;
	Elf32_Rel *rel = (void *)sechdrs[relsec].sh_addr;
	Elf32_Sym *sym;
	uint32_t *location;
	short * temp;

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
#ifdef CONFIG_CPU_CSKYV2
	uint16_t *location_tmp;
#endif

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
#ifdef CONFIG_CPU_CSKYV2
			location_tmp = (uint16_t *)location;
			if (IS_BSR32(*location_tmp, *(location_tmp + 1)))
				break;
			else if (IS_JSRI32(*location_tmp, *(location_tmp + 1))) {
				/* jsri 0x...  --> lrw r25, 0x... */
				CHANGE_JSRI_TO_LRW(location);
				/* lsli r0, r0 --> jsr r25 */
				SET_JSR32_R25(location + 1);
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
