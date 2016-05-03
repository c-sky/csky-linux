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

