/*
 *  linux/arch/csky/kernel/fiq.c
 *
 *  Copyright (C) 2010 Hu Junshan
 *
 *  FIQ support written by Hu Junsahn <junshan_hu@c-sky.com>, 2010 
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/seq_file.h>
#include <linux/rtc.h>
#include <linux/irq.h>

#include <asm/cacheflush.h>
#include <asm/fiq.h>
#include <asm/irq.h>
#include <asm/system.h>
#include <asm/machdep.h>
#include <asm/traps.h>
#include <asm/regdef.h>

int FiqStack[FIQ_STACK_SIZE] = {0, 0};    /* Fiq stack base pointer and size */
#define FiqStackBase (&FiqStack[FIQ_STACK_SIZE - 1])
 
asmlinkage void fasthandler(void);
extern asmlinkage void inthandler(void);
extern void set_evector(int vecnum, void (*handler)(void));
 
struct fiq_handler default_fiq = {
	.fiqno   = 0,
	.name    = "default",
	.handler = NULL,
};

struct fiq_controller *current_contr[NR_FIQS]; 
static struct fiq_handler *current_fiq = &default_fiq;

static void set_fiq_stack(void)
{
	unsigned long flags;
	int tmp1, tmp2;

	__save_flags(flags);
	__asm__ __volatile__("psrclr  ee, ie \n"
	                     "mfcr    %0, psr \n"
	                     "bseti   %0, 1 \n"
	                     "mtcr    %0, psr \n"
	                     "lrw     %1, %2 \n"
	                     "mov     sp, %1 \n"
	                     "mfcr    %0, psr \n"
	                     "bclri   %0, 1 \n"
	                     "mtcr    %0, psr \n"
	                     :"=b"(tmp1), "=r"(tmp2)
	                     :"i"(FiqStackBase)
	                    );
	__restore_flags(flags);
}

int show_fiq_list(struct seq_file *p, void *v)
{
    if (current_fiq != &default_fiq)
        seq_printf(p, "FIQ:         %s\n", current_fiq->name);

    return 0;
}

/*
 *  set_fiq_controller - set the fiq controller for an irq
 *   it must be called by mach_init_FIQ.
 *  @fiq:   fiq number
 *  @chip:  pointer to fiq controller description structure
 */
int set_fiq_controller(unsigned int fiq, struct fiq_controller *chip)
{

	if (!chip) {
		printk("No controller!");	
		return 0; 
	}

	current_contr[fiq - FIQ_START] = chip;

	return 0;
}

void enable_fiq(unsigned int fiq)
{
	struct fiq_controller *contr;
	
	contr = current_contr[fiq - FIQ_START];
	if(contr->startup)
		contr->startup(fiq);
	else
		contr->enable(fiq);
}

void disable_fiq(unsigned int fiq)
{
	struct fiq_controller *contr;

	contr = current_contr[fiq - FIQ_START];
	if(contr->startup)
		contr->shutdown(fiq);
	else
		contr->disable(fiq);
}

int claim_fiq(unsigned int fiq, int (*handler)(int),
       const char *devname, void *dev_id)
{
	unsigned long  fiq_enter;
	struct irq_desc *desc;

	if(fiq >= (NR_FIQS + FIQ_START)){
		printk("Bad fiq request number!");
		return 1;
	}

	/*
	 * FIXME: This judgement need to be re-considerated.
	 */	
	if(((FIQ_START - 32) == VEC_USER) && (NR_IRQS >= NR_FIQS)) {
		desc = irq_to_desc(fiq);
    	if (!desc)
			return 1;
		if(desc->action) {
			printk("IRQ %d/%s has being registered in the request number!", 
				fiq, desc->action->name);
			return 1;
		}
		/*
		 * Forbid to register common interrupt in this request number.
		 */
		desc->status |= IRQ_NOREQUEST;
	}

	fiq_enter = ((unsigned long) fasthandler) | 1;
	set_evector((int)fiq + 32, (void *)fiq_enter);
	if (current_fiq->fiqno != fiq) {
		current_fiq->fiqno   = fiq;
		current_fiq->name    = devname;
		current_fiq->handler = handler;
		current_fiq->dev_id	 = dev_id;
	}
	
	enable_fiq(fiq);
	
	return 0;
}

void release_fiq(unsigned int fiq, void *dev_id)
{
	struct irq_desc *desc;
	
	if(((FIQ_START - 32) == VEC_USER) && (NR_IRQS >= NR_FIQS)) 
	{
		desc = irq_to_desc(fiq);
		desc->status &= ~IRQ_NOREQUEST;
	}

	set_evector((int)fiq, inthandler);
	current_fiq->fiqno = 0;
	current_fiq->handler = NULL;
	disable_fiq(fiq);	
}

/*
 * csky_do_FIQ handles all hardware FIQ's.  Decoded FIQs should not
 * come via this function.  Instead, they should provide their
 * own 'handler'
 */
asmlinkage void csky_do_FIQ(unsigned int fiq)
{
	if(current_fiq->fiqno == fiq) {
		if (current_fiq->handler != NULL)
			current_fiq->handler(fiq);
		else 
			printk("No FIQ handle function!");
	}
	else 
		printk("Bad FIQ!");
}

asmlinkage void  csky_do_auto_FIQ(void)
{
	unsigned int fiq;

	if(mach_get_auto_fiqno) {
		fiq = mach_get_auto_fiqno();
	}
	else {
		printk("Error: cant get fiq number from auto FIQ!");
		return;
	}

	csky_do_FIQ(fiq);
}

EXPORT_SYMBOL(set_fiq_controller);
EXPORT_SYMBOL(claim_fiq);
EXPORT_SYMBOL(release_fiq);
EXPORT_SYMBOL(enable_fiq);
EXPORT_SYMBOL(disable_fiq);

void __init init_FIQ(void)
{
	set_fiq_stack();
	if (mach_init_FIQ)
		mach_init_FIQ();

	local_fiq_enable();
}
