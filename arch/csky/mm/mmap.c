#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/mman.h>
#include <linux/shm.h>
#include <linux/sched.h>
#include <linux/random.h>
#include <linux/io.h>

#ifdef CONFIG_MMU

unsigned long shm_align_mask = (CONFIG_CSKY_CACHE_SIZE >> 1) - 1;   /* Sane caches */

#define COLOUR_ALIGN(addr,pgoff)                            \
	((((addr) + shm_align_mask) & ~shm_align_mask) +        \
	 (((pgoff) << PAGE_SHIFT) & shm_align_mask))

unsigned long arch_get_unmapped_area(struct file *filp, unsigned long addr,
		unsigned long len, unsigned long pgoff, unsigned long flags)
{
	struct vm_area_struct * vmm;
	int do_color_align;

	if (flags & MAP_FIXED) {
		/*
		 * We do not accept a shared mapping if it would violate
		 * cache aliasing constraints.
		 */
		if ((flags & MAP_SHARED) && 
				((addr - (pgoff << PAGE_SHIFT)) & shm_align_mask))
			return -EINVAL;
		return addr;
	}

	if (len > TASK_SIZE)
		return -ENOMEM;
	do_color_align = 0;
	if (filp || (flags & MAP_SHARED))
		do_color_align = 1;
	if (addr) {
		if (do_color_align)
			addr = COLOUR_ALIGN(addr, pgoff);
		else
			addr = PAGE_ALIGN(addr);
		vmm = find_vma(current->mm, addr);
		if (TASK_SIZE - len >= addr &&
				(!vmm || addr + len <= vmm->vm_start))
			return addr;
	}
	addr = TASK_UNMAPPED_BASE;
	if (do_color_align)
		addr = COLOUR_ALIGN(addr, pgoff);
	else
		addr = PAGE_ALIGN(addr);

	for (vmm = find_vma(current->mm, addr); ; vmm = vmm->vm_next) {
		/* At this point:  (!vmm || addr < vmm->vm_end). */
		if (TASK_SIZE - len < addr)
			return -ENOMEM;
		if (!vmm || addr + len <= vmm->vm_start)
			return addr;
		addr = vmm->vm_end;
		if (do_color_align)
			addr = COLOUR_ALIGN(addr, pgoff);
	}
}

void arch_pick_mmap_layout(struct mm_struct *mm)
{
	unsigned long random_factor = 0UL;

	if (current->flags & PF_RANDOMIZE) {
		random_factor = get_random_int();
		random_factor = random_factor << PAGE_SHIFT;
		random_factor &= 0xfffffful;
	}

	mm->mmap_base = TASK_UNMAPPED_BASE + random_factor;
	mm->get_unmapped_area = arch_get_unmapped_area;
}

static inline unsigned long brk_rnd(void)
{
	unsigned long rnd = get_random_int();

	rnd = rnd << PAGE_SHIFT;
	/* 8MB for 32bit */
	rnd = rnd & 0x7ffffful;

	return rnd;
}

unsigned long arch_randomize_brk(struct mm_struct *mm)
{
	unsigned long base = mm->brk;
	unsigned long ret;

	ret = PAGE_ALIGN(base + brk_rnd());

	if (ret < mm->brk)
		return mm->brk;

	return ret;
}

#else /* !CONFIG_MMU */

#if defined(CONFIG_FB) || defined(CONFIG_FB_MODULE)
#include <linux/fb.h>
unsigned long get_fb_unmapped_area(struct file *filp, unsigned long orig_addr,
		unsigned long len, unsigned long pgoff, unsigned long flags)
{
	struct fb_info *info = filp->private_data;
	return (unsigned long)info->screen_base;
}
EXPORT_SYMBOL(get_fb_unmapped_area);
#endif

#endif /* CONFIG_MMU */
