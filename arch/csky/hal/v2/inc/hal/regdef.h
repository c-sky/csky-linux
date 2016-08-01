#ifndef  __ASM_CSKY_REGDEF_H
#define  __ASM_CSKY_REGDEF_H

#define sp		r14
#define syscallid	r7
#define a0		r0
#define a1		r1
#define a2		r2
#define a3		r3
#define regs0		r4
#define regs1		r5
#define regs2		r6
#define regs3		r7
#define regs4		r8
#define regs5		r9
#define regs6		r10
#define regs7		r11
#define regs8		r12
#define regs9		r13

/*
 * use as judge restart syscall, see entry.S
 */
#define r11_sig		r11

#endif /* __ASM_CSKY_REGDEF_H */
