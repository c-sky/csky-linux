#ifndef __ASM_CSKY_VDSO_H
#define __ASM_CSKY_VDSO_H

struct csky_vdso {
	unsigned short signal_retcode[4];
	unsigned short rt_signal_retcode[4];
};

#endif /* __ASM_CSKY_VDSO_H */
