#ifndef __ASM_CSKY_MACHDEP_H
#define __ASM_CSKY_MACHDEP_H

#include <linux/seq_file.h>
#include <linux/interrupt.h>
struct pt_regs;
struct mktime;

extern unsigned int system_rev;
extern void (*mach_time_init) (void);
/* machine dependent irq functions */
extern void (*mach_init_IRQ) (void);
extern unsigned int (*mach_get_auto_irqno) (void);
/* machine dependent timer functions */
extern unsigned long (*mach_gettimeoffset)(void);
extern void (*mach_reset)( void );
extern void (*mach_halt)( void );
extern void (*mach_power_off)( void );

extern void config_BSP(void);
extern void (*mach_tick)(void);
extern void (*mach_trap_init)(void);
extern const char *get_machine_type(void);

#endif /* __ASM_CSKY_MACHDEP_H */
