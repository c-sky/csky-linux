#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/init.h>
#include <linux/binfmts.h>
#include <linux/elf.h>
#include <linux/vmalloc.h>
#include <linux/unistd.h>

#include <asm/vdso.h>
#include <asm/uaccess.h>
#include <asm/cacheflush.h>

static struct page *vdso_page;

static int __init init_vdso(void)
{
	struct csky_vdso *vdso;
	int err = 0;

	vdso_page = alloc_page(GFP_KERNEL);
	if (!vdso_page)
		panic("Cannot allocate vdso");

	vdso = vmap(&vdso_page, 1, 0, PAGE_KERNEL);
	if (!vdso)
		panic("Cannot map vdso");

	clear_page(vdso);

#if defined(__CSKYABIV1__)
/*
 * FIXME:
 * __NR_rt_sigreturn must be 173
 * Because gcc/config/csky/linux-unwind.h
 * use hard code design,
 * and didn't use our kernel headers.
 */
	err |= __put_user(0x6000 + (127 << 4)+1, (vdso->rt_signal_retcode + 0));
	err |= __put_user(0x2000 + (31  << 4)+1, (vdso->rt_signal_retcode + 1));
	err |= __put_user(0x2000 + ((173 - 127 - 33)  << 4)+1,
					(vdso->rt_signal_retcode + 2));
	err |= __put_user(0x08, (vdso->rt_signal_retcode + 3));
#else
	/*
	 * FIXME: For CSKY V2 ISA, we mast write instruction in half word, because 
	 *  the CPU load instruction by half word and ignore endian format. So the
	 *  high half word in 32 bit instruction mast local in low address.
	 *
	 * movi r7, _NR_rt_sigreturn; trap #0 
	 */
	err |= __put_user(0xEA00 + 7, (vdso->rt_signal_retcode + 0));
	err |= __put_user(__NR_rt_sigreturn, (vdso->rt_signal_retcode + 1));
	err |= __put_user(0xC000, (vdso->rt_signal_retcode + 2));
	err |= __put_user(0x2020, (vdso->rt_signal_retcode + 3));
#endif

	if (err) panic("Cannot set signal return code, err: %x.", err);

	cache_op_range((unsigned long)vdso, ((unsigned long)vdso) + 16, DATA_CACHE|CACHE_CLR);

	vunmap(vdso);

	return 0;
}
subsys_initcall(init_vdso);

int arch_setup_additional_pages(struct linux_binprm *bprm, int uses_interp)
{
	int ret;
	unsigned long addr;
	struct mm_struct *mm = current->mm;

	down_write(&mm->mmap_sem);

	/* gary why ? */
	addr = get_unmapped_area(NULL, STACK_TOP, PAGE_SIZE, 0, 0);
	if (IS_ERR_VALUE(addr)) {
		ret = addr;
		goto up_fail;
	}

	ret = install_special_mapping(
			mm,
			addr,
			PAGE_SIZE,
			VM_READ|VM_EXEC|VM_MAYREAD|VM_MAYWRITE|VM_MAYEXEC,
			&vdso_page);
	if (ret)
		goto up_fail;

	mm->context.vdso = (void *)addr;

up_fail:
	up_write(&mm->mmap_sem);
	return ret;
}

const char *arch_vma_name(struct vm_area_struct *vma)
{
	if (vma->vm_mm == NULL)
		return NULL;

	if (vma->vm_start == (long)vma->vm_mm->context.vdso)
		return "[vdso]";
	else
		return NULL;
}

