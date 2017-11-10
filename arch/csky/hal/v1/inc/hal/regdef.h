#ifndef  __ASM_CSKY_REGDEF_H
#define  __ASM_CSKY_REGDEF_H

#define syscallid	r1
#define regs0		r6
#define regs1		r7
#define regs2		r8
#define regs3		r9
#define regs4		r10
#define regs5		r11
#define regs6		r12
#define regs7		r13
#define regs8		r14
#define regs9		r1

/*
 * use as judge restart syscall, see entry.S
 */
#define r11_sig		r11

#define DEFAULT_PSR_VALUE	0x8f000000

#endif /* __ASM_CSKY_REGDEF_H */
