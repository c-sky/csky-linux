#ifndef __ASM_CSKY_SYSTEM_H
#define __ASM_CSKY_SYSTEM_H

#include <linux/linkage.h>
#include <asm/segment.h>
#include <asm/entry.h>
#include <linux/irqflags.h>
#include <asm/regdef.h>

#ifdef __KERNEL__

/*
 * Force strict CPU ordering.
 * Not really required on csky...
 */
#define nop()  asm volatile ("nop"::)
#define mb()   asm volatile (""   : : :"memory")
#define rmb()  asm volatile (""   : : :"memory")
#define wmb()  asm volatile (""   : : :"memory")
#define set_mb(var, value)     do { var = value; mb(); } while (0)
#define set_wmb(var, value)    do { var = value; wmb(); } while (0)

#ifdef CONFIG_SMP
#define smp_mb()	mb()
#define smp_rmb()	rmb()
#define smp_wmb()	wmb()
#else
#define smp_mb()	barrier()
#define smp_rmb()	barrier()
#define smp_wmb()	barrier()
#endif

//#define smp_read_barrier_depends()      ((void)0)

#define xchg(ptr,x) ((__typeof__(*(ptr)))__xchg((unsigned long)(x),(ptr),sizeof(*(ptr))))
#define tas(ptr) (xchg((ptr),1))

struct __xchg_dummy { unsigned long a[100]; };
#define __xg(x) ((volatile struct __xchg_dummy *)(x))

static inline unsigned long __xchg(unsigned long x, volatile void * ptr, int size)
{
  unsigned long tmp, flags;

  local_irq_save(flags);

  switch (size) {
  case 1:
    __asm__ __volatile__
    ("ldb %0, %2\n\t"
     "stb %1, %2"
     : "=&r" (tmp) : "r" (x), "m" (*__xg(ptr)) : "memory");
    break;
  case 2:
    __asm__ __volatile__
    ("ldh %0, %2\n\t"
     "sth %1, %2"
    : "=&r" (tmp) : "r" (x), "m" (*__xg(ptr)) : "memory");
    break;
  case 4:
    __asm__ __volatile__
    ("ldw %0, %2\n\t"
     "stw %1,%2"
    : "=&r" (tmp) : "r" (x), "m" (*__xg(ptr)) : "memory");
    break;
  }
  local_irq_restore(flags);
  return tmp;
}

static inline unsigned long __cmpxchg(volatile unsigned long *m,
				unsigned long old, unsigned long new)
{
	unsigned long retval;
	unsigned long flags;

	local_irq_save(flags);
	retval = *m;
	if (retval == old)
		*m = new;
	local_irq_restore(flags);
	return retval;
}

#define HARD_RESET_NOW()


#endif /* __KERNEL__ */

#endif /* __ASM_CSKY_SYSTEM_H */
