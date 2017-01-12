#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>

void __delay(unsigned long loops)
{
       __asm__ __volatile__ (
#ifdef __CSKYABIV1__
				".balignw 4, 0x1200\n\t"
#else /* __CSKYABIV1__ */
                                ".balignw 4, 0x6c8b\n\t"
				"mov r0, r0\n\t"
#endif
                                "1: declt  %0\n\t"
                                "bf   1b"
                                  : "=r" (loops)
	                          : "0" (loops) );
}
EXPORT_SYMBOL(__delay);

extern unsigned long loops_per_jiffy;

void __const_udelay(unsigned long xloops)
{
	unsigned long long loops;

	loops = (unsigned long long)xloops * loops_per_jiffy * HZ;

	__delay(loops >> 32);
}
EXPORT_SYMBOL(__const_udelay);

void __udelay(unsigned long usecs)
{
	__const_udelay(usecs * 0x10C7UL); /* 2**32 / 1000000 (rounded up) */
}
EXPORT_SYMBOL(__udelay);

void __ndelay(unsigned long nsecs)
{
	__const_udelay(nsecs * 0x5UL); /* 2**32 / 1000000000 (rounded up) */
}
EXPORT_SYMBOL(__ndelay);
