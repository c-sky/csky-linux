#ifndef __ASM_CSKY_ADDRSPACE_H
#define __ASM_CSKY_ADDRSPACE_H

/*
 * Memory segments
 */
#define KSEG0			0x80000000ul
#define KSEG1			0xa0000000ul
#define KSEG2			0xc0000000ul
#define KSEG3			0xe0000000ul

#define CPHYSADDR(a)		(((unsigned long) (a)) & 0x1fffffff)

#define KSEG0ADDR(a)		(CPHYSADDR(a) | KSEG0)
#define KSEG1ADDR(a)		(CPHYSADDR(a) | KSEG1)
#define KSEG2ADDR(a)		(CPHYSADDR(a) | KSEG2)
#define KSEG3ADDR(a)		(CPHYSADDR(a) | KSEG3)

#endif /* __ASM_CSKY_ADDRSPACE_H */

