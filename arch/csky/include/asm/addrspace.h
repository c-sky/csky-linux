#ifndef __ASM_CSKY_ADDRSPACE_H
#define __ASM_CSKY_ADDRSPACE_H

/* Cached for ram */
#define KSEG0		0x80000000ul
#define KSEG0ADDR(a)	(((unsigned long)a & 0x1fffffff) | KSEG0)

/* UnCached for io */
#define KSEG1		0xa0000000ul
#define KSEG1ADDR(a)	(((unsigned long)a & 0x1fffffff) | KSEG1)

#endif /* __ASM_CSKY_ADDRSPACE_H */

