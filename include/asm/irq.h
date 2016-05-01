#ifndef __ASM_CSKY_IRQ_H
#define __ASM_CSKY_IRQ_H

#include <mach/irqs.h>

#include <linux/linkage.h>
#include <linux/hardirq.h>
#include <linux/irqreturn.h>
#include <linux/spinlock_types.h>

/*
 * "Generic" interrupt sources
 */

#define IRQ_SCHED_TIMER	(8)    /* interrupt source for scheduling timer */

static __inline__ int irq_canonicalize(int irq)
{
	return irq;
}

/*
 * Machine specific interrupt sources.
 *
 * Adding an interrupt service routine for a source with this bit
 * set indicates a special machine specific interrupt source.
 * The machine specific files define these sources.
 *
 * The IRQ_MACHSPEC bit is now gone - the only thing it did was to
 * introduce unnecessary overhead.
 *
 * All interrupt handling is actually machine specific so it is better
 * to use function pointers, as used by the Sparc port, and select the
 * interrupt handling functions when initializing the kernel. This way
 * we save some unnecessary overhead at run-time. 
 *                                                      01/11/97 - Jes
 */

extern void (*mach_enable_irq)(unsigned int);
extern void (*mach_disable_irq)(unsigned int);


#define IRQ_MACHSPEC  (0x10000000L)
#define IRQ_IDX(irq)  ((irq) & ~IRQ_MACHSPEC)
/*
 * various flags for request_irq() - the Amiga now uses the standard
 * mechanism like all other architectures - SA_INTERRUPT and SA_SHIRQ
 * are your friends.
 */
#define IRQ_FLG_LOCK	(0x0001)	/* handler is not replaceable	*/
#define IRQ_FLG_REPLACE	(0x0002)	/* replace existing handler	*/
#define IRQ_FLG_FAST	(0x0004)
#define IRQ_FLG_SLOW	(0x0008)
#define IRQ_FLG_STD	(0x8000)	/* internally used		*/



/*
 * This structure is used to chain together the ISRs for a particular
 * interrupt source (if it supports chaining).
 */
typedef struct irq_node {
	void		(*handler)(int, void *, struct pt_regs *);
	unsigned long	flags;
	void		*dev_id;
	const char	*devname;
	struct irq_node *next;
} irq_node_t;

/* count of spurious interrupts */
extern volatile unsigned int num_spurious;

#endif /* __ASM_CSKY_IRQ_H */
