#ifndef __ASM_CSKY_MISC_H
#define __ASM_CSKY_MISC_H
#include <asm/ptrace.h>
inline unsigned int read_pt_regs(unsigned int rx, struct pt_regs *regs);
inline void write_pt_regs(unsigned int value, unsigned int rx, struct pt_regs *regs);
inline unsigned int read_fpcr(void);
inline void write_fpcr(unsigned int val);
inline unsigned int read_fpesr(void);
inline void write_fpesr(unsigned int val);
#ifdef __CSKYABIV1__
inline unsigned int read_fpsr(void);
inline void write_fpsr(unsigned int val);
#endif
#endif /* __CSKY_MISC_H__ */
