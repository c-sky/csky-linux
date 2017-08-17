#ifndef __ASM_CSKY_MISC_H
#define __ASM_CSKY_MISC_H
#include <asm/ptrace.h>
unsigned int read_pt_regs(unsigned int rx, struct pt_regs *regs);
void write_pt_regs(unsigned int value, unsigned int rx, struct pt_regs *regs);
unsigned int read_fpcr(void);
void write_fpcr(unsigned int val);
unsigned int read_fpesr(void);
void write_fpesr(unsigned int val);
#ifdef __CSKYABIV1__
unsigned int read_fpsr(void);
void write_fpsr(unsigned int val);
#endif
#endif /* __CSKY_MISC_H__ */
