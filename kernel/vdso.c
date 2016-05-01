/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2013 C-SKY Microsystem Co.,Ltd
 * Author: Hu Junshan (junshan_hu@c-sky.com) 
 */

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

#if defined(CONFIG_CPU_CSKYV1)
	/* movi r1, __NR_sigreturn; trap 0 */
	err |= __put_user(0x6000 + (__NR_sigreturn << 4) + 1,
	                     (vdso->signal_retcode + 0));
	err |= __put_user(0x08, (vdso->signal_retcode + 1));
	/* 
	 * movi r1,127 
	 * addi r1,33 
	 * addi r1,(__NR_sigreturn-127-33)   // __NR_rt_sigreturn
	 * trap 0 
	 */
	err |= __put_user(0x6000 + (127 << 4)+1, (vdso->rt_signal_retcode + 0));
	err |= __put_user(0x2000 + (31  << 4)+1, (vdso->rt_signal_retcode + 1));
	err |= __put_user(0x2000 + ((__NR_rt_sigreturn-127-33)  << 4)+1,
	        (vdso->rt_signal_retcode + 2));
	err |= __put_user(0x08, (vdso->rt_signal_retcode + 3));
#else
	/*
	 * FIXME: For CSKY V2 ISA, we mast write instruction in half word, because 
	 *  the CPU load instruction by half word and ignore endian format. So the
	 *  high half word in 32 bit instruction mast local in low address.
	 *
	 * movi r7, _NR_sigreturn; trap #0 
	 */
	err |= __put_user(0xEA00 + 7, (vdso->signal_retcode + 0));
	err |= __put_user(__NR_sigreturn, (vdso->signal_retcode + 1));
	err |= __put_user(0xC000, (vdso->signal_retcode + 2));
	err |= __put_user(0x2020, (vdso->signal_retcode + 3));
	/* movi r7, __NR_rt_sigreturn; trap #0 */
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

static unsigned long vdso_addr(unsigned long start)
{
	return STACK_TOP;
}

int arch_setup_additional_pages(struct linux_binprm *bprm, int uses_interp)
{
	int ret;
	unsigned long addr;
	struct mm_struct *mm = current->mm;

	down_write(&mm->mmap_sem);

	addr = vdso_addr(mm->start_stack);

	addr = get_unmapped_area(NULL, addr, PAGE_SIZE, 0, 0);
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
