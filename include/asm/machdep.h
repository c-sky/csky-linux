#ifndef __ASM_CSKY_MACHDEP_H
#define __ASM_CSKY_MACHDEP_H

#include <linux/seq_file.h>
#include <linux/interrupt.h>

extern void (*mach_init_IRQ) (void);
extern unsigned int (*mach_get_auto_irqno) (void);

extern void (*mach_time_init) (void);

extern void config_BSP(void);
extern void csky_tick(void);

#endif /* __ASM_CSKY_MACHDEP_H */
