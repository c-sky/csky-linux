#ifndef __ASM_ARCH_UNCOMPRESS_H
#define __ASM_ARCH_UNCOMPRESS_H

#define putc(c) \
do { \
	while (!((*((volatile unsigned int *)(0xa0400000 + 0x04))) & (1 << 7))); \
	*((volatile unsigned int *)(0xa0400000 + 0x08)) = (unsigned int)c; \
} while(0)

static inline void flush(void)
{
}

void inline uart_init(void)
{
}

void inline putcu(unsigned char c)
{
	putc(c);
}
#define arch_decomp_setup()
#define arch_decomp_wdog()

#endif /* __ASM_ARCH_UNCOMPRESS_H */
