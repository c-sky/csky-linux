/*
 * linux/arch/csky/kernel/sys_csky.c
 *
 * This file contains various random system calls that
 * have a non-standard calling sequence on the Linux/csky platform.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2006  Hangzhou C-SKY Microsystems co.,ltd.
 * Copyright (C) 2006  Li Chunqiang (chunqiang_li@c-sky.com)
 * Copyright (C) 2009  Hu junshan (junshan_hu@c-sky.com)  
 *
 */
#include <linux/capability.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/smp.h>
#include <linux/sem.h>
#include <linux/msg.h>
#include <linux/shm.h>
#include <linux/stat.h>
#include <linux/syscalls.h>
#include <linux/mman.h>
#include <linux/file.h>
#include <linux/utsname.h>
#include <linux/ipc.h>

#include <asm/setup.h>
#include <asm/uaccess.h>
#include <asm/traps.h>
#include <asm/unistd.h>
#include <asm/regdef.h>
#include <asm/prfl.h>
#include <asm/user.h>

/* common code for old and new mmaps */
static inline long do_mmap2(
	unsigned long addr, unsigned long len,
	unsigned long prot, unsigned long flags,
	unsigned long fd, unsigned long pgoff)
{
	int error = -EBADF;
	struct file * file = NULL;
	unsigned long unused;

	flags &= ~(MAP_EXECUTABLE | MAP_DENYWRITE);
	if (!(flags & MAP_ANONYMOUS)) {
		file = fget(fd);
		if (!file)
			goto out;
	}

	down_write(&current->mm->mmap_sem);
	error = do_mmap_pgoff(file, addr, len, prot, flags, pgoff, &unused);
	up_write(&current->mm->mmap_sem);

	if (file)
		fput(file);
out:
	return error;
}

asmlinkage long sys_mmap2(unsigned long addr, unsigned long len,
	unsigned long prot, unsigned long flags,
	unsigned long fd, unsigned long pgoff)
{
	return do_mmap2(addr, len, prot, flags, fd, pgoff);
}

/*
 * Perform the select(nd, in, out, ex, tv) and mmap() system
 * calls. Linux/csky cloned Linux/i386, which didn't use to be able to
 * handle more than 4 system call parameters, so these system calls
 * used a memory block for parameter passing..
 */

struct mmap_arg_struct {
	unsigned long addr;
	unsigned long len;
	unsigned long prot;
	unsigned long flags;
	unsigned long fd;
	unsigned long offset;
};

asmlinkage int old_mmap(struct mmap_arg_struct *arg)
{
	struct mmap_arg_struct a;
	int error = -EFAULT;

	if (copy_from_user(&a, arg, sizeof(a)))
		goto out;

	error = -EINVAL;
	if (a.offset & ~PAGE_MASK)
		goto out;

	a.flags &= ~(MAP_EXECUTABLE | MAP_DENYWRITE);

	error = do_mmap2(a.addr, a.len, a.prot, a.flags, a.fd, a.offset >> PAGE_SHIFT);
out:
	return error;
}



struct sel_arg_struct {
	unsigned long n;
	fd_set *inp, *outp, *exp;
	struct timeval *tvp;
};

asmlinkage int old_select(struct sel_arg_struct *arg)
{
	struct sel_arg_struct a;

	if (copy_from_user(&a, arg, sizeof(a)))
		return -EFAULT;
	/* sys_select() does the appropriate kernel locking */
	return sys_select(a.n, a.inp, a.outp, a.exp, a.tvp);
}

asmlinkage int sys_getpagesize(void)
{
	return PAGE_SIZE;
}

/*              
 * Do a system call from kernel instead of calling sys_execve so we
 * end up with proper pt_regs.
 */     
int kernel_execve(const char *filename, const char *const argv[], 
	const char *const envp[])
{
        register long __res;
        __asm__ __volatile__(
#if defined(__CSKYABIV2__) 
 		"movi   r7, %4\n\t"
#else
  		"movi	r1, %4\n\t"
#endif
		"mov    a0, %1\n\t"
                "mov    a1, %2\n\t"
                "mov    a2, %3\n\t"
                "trap   0     \n\t"
                "mov    %0, a0\n\t"
                : "=r" (__res)
                : "r" (filename), 
                  "r" (argv), 
                  "r" (envp),
                  "i" (__NR_execve)
#if defined(__CSKYABIV2__)
		: "r7", "a0", "a1", "a2");
#else
		: "r1", "a0", "a1", "a2");        
#endif
        return __res;
}

/*
 * Since loff_t is a 64 bit type we avoid a lot of ABI hassle
 * with a different argument ordering.
 */
asmlinkage long sys_csky_fadvise64_64(int fd, int advice,
                                     loff_t offset, loff_t len)
{
	return sys_fadvise64_64(fd, offset, len, advice);
}

asmlinkage long csky_prfl_trig(unsigned int __user spc, unsigned int __user epc, unsigned int __user u_s_softcount_enable)
{
#if defined(CONFIG_CPU_CSKYV2)                      
	unsigned long flg;
	unsigned int uce, sce, hpsir, hpcr;

	hpcr = (HPCR_ECR | HPCR_HPCCR | HPCR_TCE | HPCR_HPCCE | HPCR_ECE);
	hpsir = u_s_softcount_enable % 10;
	sce = u_s_softcount_enable / 10 % 10;
	uce = u_s_softcount_enable / 100;
	if(uce == 1)
		hpcr |= uce << 3;
	if(sce == 1)
		hpcr |= sce << 2;
	local_save_flags(flg);
	__asm__ __volatile__("cpwcr   %0, <0, 0x0>\n\t"
	                     "cpwcr   %1, <0, 0x1>\n\t"
	                     "cpwcr   %2, <0, 0x2>\n\t"
	                     "cpwcr   %3, <0, 0x3>\n\t"
	                     ::"r"(hpcr),"r"(spc),"r"(epc),"r"(hpsir));
	local_irq_restore(flg);
#endif
	return 1;
}

asmlinkage long csky_prfl_read(struct user_cskyprfl_struct __user *prfl)
{
#if defined(CONFIG_CPU_CSKYV2)
	__asm__ __volatile__("cprcr  %0, <0, 0x0>\n\r"
	                        :"=r"(prfl->hpcr));
	__asm__ __volatile__("cprcr  %0, <0, 0x1>\n\r"
	                        :"=r"(prfl->hpspr));
	__asm__ __volatile__("cprcr  %0, <0, 0x2>\n\r"
	                        :"=r"(prfl->hpepr));
	__asm__ __volatile__("cprcr  %0, <0, 0x3>\n\r"
	                        :"=r"(prfl->hpsir));
	__asm__ __volatile__("cprgr  %0, <0, 0x0>\n\r"
	                        :"=r"(prfl->soft[0]));
	__asm__ __volatile__("cprgr  %0, <0, 0x1>\n\r"
	                        :"=r"(prfl->soft[1]));
	__asm__ __volatile__("cprgr  %0, <0, 0x2>\n\r"
	                        :"=r"(prfl->soft[2]));
	__asm__ __volatile__("cprgr  %0, <0, 0x3>\n\r"
	                        :"=r"(prfl->soft[3]));
	__asm__ __volatile__("cprgr  %0, <0, 0x4>\n\r"
	                        :"=r"(prfl->soft[4]));
	__asm__ __volatile__("cprgr  %0, <0, 0x5>\n\r"
	                        :"=r"(prfl->soft[5]));
	__asm__ __volatile__("cprgr  %0, <0, 0x6>\n\r"
	                        :"=r"(prfl->soft[6]));
	__asm__ __volatile__("cprgr  %0, <0, 0x7>\n\r"
	                        :"=r"(prfl->soft[7]));
	__asm__ __volatile__("cprgr  %0, <0, 0x8>\n\r"
	                        :"=r"(prfl->soft[8]));
	__asm__ __volatile__("cprgr  %0, <0, 0x9>\n\r"
	                        :"=r"(prfl->soft[9]));
	__asm__ __volatile__("cprgr  %0, <0, 0xa>\n\r"
	                        :"=r"(prfl->soft[10]));
	__asm__ __volatile__("cprgr  %0, <0, 0xb>\n\r"
	                        :"=r"(prfl->soft[11]));
	__asm__ __volatile__("cprgr  %0, <0, 0xc>\n\r"
	                        :"=r"(prfl->soft[12]));
	__asm__ __volatile__("cprgr  %0, <0, 0xd>\n\r"
	                        :"=r"(prfl->soft[13]));
	__asm__ __volatile__("cprgr  %0, <0, 0xe>\n\r"
	                        :"=r"(prfl->soft[14]));
	__asm__ __volatile__("cprgr  %0, <0, 0xf>\n\r"
	                        :"=r"(prfl->soft[15]));
	__asm__ __volatile__("cprgr  %0, <0, 0x10>\n\r"
	                        :"=r"(prfl->soft[16]));
	__asm__ __volatile__("cprgr  %0, <0, 0x11>\n\r"
	                        :"=r"(prfl->soft[17]));
	__asm__ __volatile__("cprgr  %0, <0, 0x12>\n\r"
	                        :"=r"(prfl->soft[18]));
	__asm__ __volatile__("cprgr  %0, <0, 0x13>\n\r"
	                        :"=r"(prfl->soft[19]));
	__asm__ __volatile__("cprgr  %0, <0, 0x14>\n\r"
	                        :"=r"(prfl->soft[20]));
	__asm__ __volatile__("cprgr  %0, <0, 0x15>\n\r"
	                        :"=r"(prfl->soft[21]));
	__asm__ __volatile__("cprgr  %0, <0, 0x16>\n\r"
	                        :"=r"(prfl->soft[22]));
	__asm__ __volatile__("cprgr  %0, <0, 0x17>\n\r"
	                        :"=r"(prfl->soft[23]));
	__asm__ __volatile__("cprgr  %0, <0, 0x18>\n\r"
	                        :"=r"(prfl->soft[24]));
	__asm__ __volatile__("cprgr  %0, <0, 0x19>\n\r"
	                        :"=r"(prfl->soft[25]));
	__asm__ __volatile__("cprgr  %0, <0, 0x1a>\n\r"
	                        :"=r"(prfl->soft[26]));
	__asm__ __volatile__("cprgr  %0, <0, 0x1b>\n\r"
	                        :"=r"(prfl->soft[27]));
	__asm__ __volatile__("cprgr  %0, <0, 0x20>\n\r"
	                        :"=r"(prfl->hard[0]));
	__asm__ __volatile__("cprgr  %0, <0, 0x21>\n\r"
	                        :"=r"(prfl->hard[1]));
	__asm__ __volatile__("cprgr  %0, <0, 0x22>\n\r"
	                        :"=r"(prfl->hard[2]));
	__asm__ __volatile__("cprgr  %0, <0, 0x23>\n\r"
	                        :"=r"(prfl->hard[3]));
	__asm__ __volatile__("cprgr  %0, <0, 0x24>\n\r"
	                        :"=r"(prfl->hard[4]));
	__asm__ __volatile__("cprgr  %0, <0, 0x25>\n\r"
	                        :"=r"(prfl->hard[5]));
	__asm__ __volatile__("cprgr  %0, <0, 0x26>\n\r"
	                        :"=r"(prfl->hard[6]));
	__asm__ __volatile__("cprgr  %0, <0, 0x27>\n\r"
	                        :"=r"(prfl->hard[7]));
	__asm__ __volatile__("cprgr  %0, <0, 0x28>\n\r"
	                        :"=r"(prfl->hard[8]));
	__asm__ __volatile__("cprgr  %0, <0, 0x29>\n\r"
	                        :"=r"(prfl->hard[9]));
	__asm__ __volatile__("cprgr  %0, <0, 0x2a>\n\r"
	                        :"=r"(prfl->hard[10]));
	__asm__ __volatile__("cprgr  %0, <0, 0x2b>\n\r"
	                        :"=r"(prfl->hard[11]));
	__asm__ __volatile__("cprgr  %0, <0, 0x2c>\n\r"
	                        :"=r"(prfl->hard[12]));
	__asm__ __volatile__("cprgr  %0, <0, 0x2d>\n\r"
	                        :"=r"(prfl->hard[13]));
	__asm__ __volatile__("cprgr  %0, <0, 0x2e>\n\r"
	                        :"=r"(prfl->hard[14]));
	__asm__ __volatile__("cprgr  %0, <0, 0x2f>\n\r"
	                        :"=r"(prfl->hard[15]));
	__asm__ __volatile__("cprgr  %0, <0, 0x30>\n\r"
	                        :"=r"(prfl->hard[16]));
	__asm__ __volatile__("cprgr  %0, <0, 0x31>\n\r"
	                        :"=r"(prfl->hard[17]));
	__asm__ __volatile__("cprgr  %0, <0, 0x32>\n\r"
	                        :"=r"(prfl->hard[18]));
	__asm__ __volatile__("cprgr  %0, <0, 0x33>\n\r"
	                        :"=r"(prfl->hard[19]));
	__asm__ __volatile__("cprgr  %0, <0, 0x34>\n\r"
	                        :"=r"(prfl->hard[20]));
	__asm__ __volatile__("cprgr  %0, <0, 0x35>\n\r"
	                        :"=r"(prfl->hard[21]));
	__asm__ __volatile__("cprgr  %0, <0, 0x36>\n\r"
	                        :"=r"(prfl->hard[22]));
	__asm__ __volatile__("cprgr  %0, <0, 0x37>\n\r"
	                        :"=r"(prfl->hard[23]));
	__asm__ __volatile__("cprgr  %0, <0, 0x38>\n\r"
	                        :"=r"(prfl->hard[24]));
	__asm__ __volatile__("cprgr  %0, <0, 0x39>\n\r"
	                        :"=r"(prfl->hard[25]));
	__asm__ __volatile__("cprgr  %0, <0, 0x3a>\n\r"
	                        :"=r"(prfl->hard[26]));
	__asm__ __volatile__("cprgr  %0, <0, 0x3b>\n\r"
	                        :"=r"(prfl->hard[27]));
	__asm__ __volatile__("cprgr  %0, <0, 0x3c>\n\r"
	                        :"=r"(prfl->hard[28]));
	__asm__ __volatile__("cprgr  %0, <0, 0x3d>\n\r"
	                        :"=r"(prfl->hard[29]));
	__asm__ __volatile__("cprgr  %0, <0, 0x40>\n\r"
	                        :"=r"(prfl->extend[0]));
	__asm__ __volatile__("cprgr  %0, <0, 0x41>\n\r"
	                        :"=r"(prfl->extend[1]));
	__asm__ __volatile__("cprgr  %0, <0, 0x42>\n\r"
	                        :"=r"(prfl->extend[2]));
	__asm__ __volatile__("cprgr  %0, <0, 0x43>\n\r"
	                        :"=r"(prfl->extend[3]));
	__asm__ __volatile__("cprgr  %0, <0, 0x44>\n\r"
	                        :"=r"(prfl->extend[4]));
	 __asm__ __volatile__("cprgr  %0, <0, 0x45>\n\r"
	                        :"=r"(prfl->extend[5]));
	__asm__ __volatile__("cprgr  %0, <0, 0x46>\n\r"
	                        :"=r"(prfl->extend[6]));
	__asm__ __volatile__("cprgr  %0, <0, 0x47>\n\r"
	                        :"=r"(prfl->extend[7]));
	__asm__ __volatile__("cprgr  %0, <0, 0x48>\n\r"
	                        :"=r"(prfl->extend[8]));
	__asm__ __volatile__("cprgr  %0, <0, 0x49>\n\r"
	                        :"=r"(prfl->extend[9]));
	 __asm__ __volatile__("cprgr  %0, <0, 0x4a>\n\r"
	                        :"=r"(prfl->extend[10]));
	__asm__ __volatile__("cprgr  %0, <0, 0x4b>\n\r"
	                        :"=r"(prfl->extend[11]));
	__asm__ __volatile__("cprgr  %0, <0, 0x4c>\n\r"
	                        :"=r"(prfl->extend[12]));
	__asm__ __volatile__("cprgr  %0, <0, 0x4d>\n\r"
	                        :"=r"(prfl->extend[13]));
	__asm__ __volatile__("cprgr  %0, <0, 0x4e>\n\r"
	                        :"=r"(prfl->extend[14]));
	 __asm__ __volatile__("cprgr  %0, <0, 0x4f>\n\r"
	                        :"=r"(prfl->extend[15]));
	__asm__ __volatile__("cprgr  %0, <0, 0x50>\n\r"
	                        :"=r"(prfl->extend[16]));
	__asm__ __volatile__("cprgr  %0, <0, 0x51>\n\r"
	                        :"=r"(prfl->extend[17]));
	__asm__ __volatile__("cprgr  %0, <0, 0x52>\n\r"
	                        :"=r"(prfl->extend[18]));
	__asm__ __volatile__("cprgr  %0, <0, 0x53>\n\r"
	                        :"=r"(prfl->extend[19]));
	 __asm__ __volatile__("cprgr  %0, <0, 0x54>\n\r"
	                        :"=r"(prfl->extend[20]));
	__asm__ __volatile__("cprgr  %0, <0, 0x55>\n\r"
	                        :"=r"(prfl->extend[21]));
	__asm__ __volatile__("cprgr  %0, <0, 0x56>\n\r"
	                        :"=r"(prfl->extend[22]));
	__asm__ __volatile__("cprgr  %0, <0, 0x57>\n\r"
	                        :"=r"(prfl->extend[23]));
	__asm__ __volatile__("cprgr  %0, <0, 0x58>\n\r"
	                        :"=r"(prfl->extend[24]));
	__asm__ __volatile__("cprgr  %0, <0, 0x59>\n\r"
	                        :"=r"(prfl->extend[25]));
#if 0
	printk("hpcr:0x%08lx  hpspr:0x%08lx  hpepr:0x%08lx  hpsir:0x%08lx\n", prfl->hpcr, prfl->hpspr, prfl->hpepr, prfl->hpsir);
	printk("        -----Software Event count!!!-----\n");
	printk("Hardware Profling Cycle Counter:%08ld-%08ld  Cycle Counter:%08ld-%08ld Total Instruction Counter:%08ld-%08ld\n", prfl->soft[1], prfl->soft[0], prfl->soft[3], prfl->soft[2], prfl->soft[5], prfl->soft[4]);
	printk("Change Flow Instruction Counter:%08ld-%08ld  Load Instruction Counter:%08ld-%08ld  Store Instruction Counter: %08ld-%08ld\n", prfl->soft[7], prfl->soft[6], prfl->soft[9], prfl->soft[8], prfl->soft[11], prfl->soft[10]);
	printk("Function Return Instruction Counter:%08ld-%08ld  Conditional Branch Instruction Counter:%08ld-%08ld  Immediate Branch Instruction Counter:%08ld-%08ld\n", prfl->soft[13], prfl->soft[12], prfl->soft[15], prfl->soft[14], prfl->soft[17], prfl->soft[16]);
	printk("Processed Interrupt Counter:%08ld-%08ld  Processed Exception Counter:%08ld-%08ld  System Call Counter:%08ld-%08ld\n", prfl->soft[19], prfl->soft[18], prfl->soft[21],  prfl->soft[20], prfl->soft[23], prfl->soft[22]);
	printk("Interrupt Mask Cycle Counter:%08ld-%08ld  Software Counter:%08ld-%08ld\n", prfl->soft[25], prfl->soft[24], prfl->soft[27], prfl->soft[26]);
	printk("        -----Hardware Event count!!!-----\n");
	printk("L0/L1 ICache Access Times:%08ld-%08ld  L1-ICache Miss Times: %08ld-%08ld  L1-DCache Access Times: %08ld-%08ld\n", prfl->hard[1], prfl->hard[0], prfl->hard[3], prfl->hard[2], prfl->hard[5], prfl->hard[4]);
	printk("L1-DCache Miss Times:%08ld-%08ld  Non-Cache Load/Store:%08ld-%08ld  I-UTLB Miss Times:%08ld-%08ld\n", prfl->hard[7], prfl->hard[6], prfl->hard[9], prfl->hard[8], prfl->hard[11], prfl->hard[10]);
	printk("D-UTLB Miss Times:%08ld-%08ld  JTLB Miss Times:%08ld-%08ld  Branch Mispredict(BHT/BTB/RAS)Times:%08ld-%08ld\n", prfl->hard[13], prfl->hard[12], prfl->hard[15], prfl->hard[14], prfl->hard[17], prfl->hard[16]);
	printk("Fetch No Instruction Cycle Counter:%08ld-%08ld  Total Stall Cycle Counter:%08ld-%08ld  Resource Unavailable Stall Cycle Counter:%08ld-%08ld\n", prfl->hard[19], prfl->hard[18], prfl->hard[21], prfl->hard[20], prfl->hard[23], prfl->hard[22]);
	printk("IBUS Wait Cycle:%08ld-%08ld  DBUS Wait Cycle:%08ld-%08ld  BUS Ungrant Cycle:%08ld-%08ld\n", prfl->hard[25], prfl->hard[24], prfl->hard[27], prfl->hard[26], prfl->hard[29], prfl->hard[28]);
	printk("        -----extended Event count!!!-----\n");
	printk("Issue Queue Stall Cycle:%08ld-%08ld  ROB Stall Cycle:%08ld-%08ld  LSPB Stall Cycle:%08ld-%08ld\n", prfl->extend[1], prfl->extend[0], prfl->extend[3], prfl->extend[2], prfl->extend[5], prfl->extend[4]);
	printk("RQ Stall Cycle:%08ld-%08ld  MDRQ Stall Cycle:%08ld-%08ld  Instruction Buffer Stall Cycle:%08ld-%08ld\n", prfl->extend[7], prfl->extend[6], prfl->extend[9], prfl->extend[8], prfl->extend[11], prfl->extend[10]);
	printk("PCFIFO Stall Counter:%08ld-%08ld  Split Instruction Counter:%08ld-%08ld  BHT Mispredict Times:%08ld-%08ld\n", prfl->extend[13], prfl->extend[12], prfl->extend[15], prfl->extend[14], prfl->extend[17], prfl->extend[16]);
	printk("BTB Mispredict:%08ld-%08ld  RAS Mispredict:%08ld-%08ld  L0-ICache Miss Times:%08ld-%08ld\n", prfl->extend[19], prfl->extend[18], prfl->extend[21], prfl->extend[20], prfl->extend[23], prfl->extend[22]);
	printk("LSU Spec Fail Times:0x%08lx-0x%08lx\n", prfl->extend[25], prfl->extend[24]);
#endif
#endif /* CONFIG_CPU_CSKYV2 */
	return 1;
}
