#ifndef __ASM_CSKY_VDSO_H
#define __ASM_CSKY_VDSO_H

struct csky_vdso {
	unsigned short signal_retcode[4];
	unsigned short rt_signal_retcode[4];
};

#ifndef CONFIG_MMU
extern struct csky_vdso *global_vdso;
#endif

#endif /* __ASM_CSKY_VDSO_H */
