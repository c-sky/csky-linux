#ifndef __ASM_CSKY_MMU_CONTEXT_H
#define __ASM_CSKY_MMU_CONTEXT_H

#include <asm-generic/mm_hooks.h>
#include <asm/setup.h>
#include <asm/page.h>
#include <asm/cacheflush.h>
#include <asm/tlbflush.h>

#include <linux/errno.h>
#include <linux/sched.h>

#ifdef CONFIG_CPU_MMU_V1
#include <asm/ckmmuv1.h>
#else
#include <asm/ckmmuv2.h>
#endif

/*
 * For the fast tlb miss handlers, we currently keep a per cpu array
 * of pointers to the current pgd for each processor. Also, the proc.
 * id is stuffed into the context register. This should be changed to
 * use the processor id via current->processor, where current is stored
 * in watchhi/lo. The context register should be used to contiguously
 * map the page tables.
 */
#ifdef CONFIG_MMU_HARD_REFILL
#ifdef CONFIG_CPU_MMU_V1
#define TLBMISS_HANDLER_SETUP_PGD(pgd)                \
do{                                                   \
	__asm__ __volatile__(                         \
	            "       mov   r6, %0 \n"          \
	            "       bseti r6, 0 \n"           \
	            "       bclri r6, 31 \n"          \
	            "       addu  r6, %1 \n"          \
	            "       cpseti    cp15 \n"        \
	            "       cpwcr r6, cpcr29 \n"      \
	            ::"r"(pgd), "r"(PHYS_OFFSET)      \
	            :"r6");                           \
}while(0)
#else
#define TLBMISS_HANDLER_SETUP_PGD(pgd)                \
do{                                                   \
	__asm__ __volatile__(                         \
	            "       bseti %0, 0 \n"           \
	            "       bclri %0, 31 \n"          \
	            "       addu  %0, %1 \n"          \
	            "       mtcr  %0, cr<29, 15> \n"  \
	            ::"r"(pgd), "r"(PHYS_OFFSET)      \
	            :);                               \
}while(0)
#endif /* CONFIG_CPU_MMU_V1 */

#define TLBMISS_HANDLER_SETUP() \
    TLBMISS_HANDLER_SETUP_PGD(swapper_pg_dir)
#else
#define TLBMISS_HANDLER_SETUP_PGD(pgd) \
	pgd_current[smp_processor_id()] = (unsigned long)(pgd)
#define TLBMISS_HANDLER_SETUP() \
	TLBMISS_HANDLER_SETUP_PGD(swapper_pg_dir)
extern unsigned long pgd_current[];
#endif /* CONFIG_MMU_HARD_REFILL */

#define cpu_context(cpu, mm)	((mm)->context.asid[cpu])
#define cpu_asid(cpu, mm)	(cpu_context((cpu), (mm)) & ASID_MASK)
#define asid_cache(cpu)		(cpu_data[cpu].asid_cache)

#define ASID_INC		0x1
#define ASID_MASK		0xff
#define ASID_VERSION_MASK	0xffffff00
#define ASID_FIRST_VERSION	0x100

static inline void enter_lazy_tlb(struct mm_struct *mm, struct task_struct *tsk)
{
}

/*
 *  All unused by hardware upper bits will be considered
 *  as a software asid extension.
 */
#ifdef CONFIG_MMU
static inline void
get_new_mmu_context(struct mm_struct *mm, unsigned long cpu)
{
	unsigned long asid = asid_cache(cpu);

	if (! ((asid += ASID_INC) & ASID_MASK) ) {
		flush_cache_all();
		local_flush_tlb_all();	/* start new asid cycle */
		if (!asid)		/* fix version if needed */
			asid = ASID_FIRST_VERSION;
	}
	cpu_context(cpu, mm) = asid_cache(cpu) = asid;
}
#endif

/*
 * Initialize the context related info for a new mm_struct
 * instance.
 */
static inline int
init_new_context(struct task_struct *tsk, struct mm_struct *mm)
{
#ifdef CONFIG_MMU
	int i;

	for_each_online_cpu(i)
		cpu_context(i, mm) = 0;
#endif
	return 0;
}

static inline void switch_mm(struct mm_struct *prev, struct mm_struct *next,
                             struct task_struct *tsk)
{
#ifdef CONFIG_MMU
	unsigned int cpu = smp_processor_id();
	unsigned long flags;

	local_irq_save(flags);
	/* Check if our ASID is of an older version and thus invalid */
	if ((cpu_context(cpu, next) ^ asid_cache(cpu)) & ASID_VERSION_MASK)
		get_new_mmu_context(next, cpu);
	write_mmu_entryhi(cpu_context(cpu, next));
	TLBMISS_HANDLER_SETUP_PGD(next->pgd);

	/*
	 * Mark current->active_mm as not "active" anymore.
	 * We don't want to mislead possible IPI tlb flush routines.
	 */
	cpumask_clear_cpu(cpu, mm_cpumask(prev));
	cpumask_set_cpu(cpu, mm_cpumask(next));

	local_irq_restore(flags);
#endif
}

/*
 * Destroy context related info for an mm_struct that is about
 * to be put to rest.
 */
static inline void destroy_context(struct mm_struct *mm)
{
}

/*
 * After we have set current->mm to a new value, this activates
 * the context for the new mm so we see the new mappings.
 */
static inline void
activate_mm(struct mm_struct *prev, struct mm_struct *next)
{
#ifdef CONFIG_MMU
	unsigned long flags;
	int cpu = smp_processor_id();

	local_irq_save(flags);

	/* Unconditionally get a new ASID.  */
	get_new_mmu_context(next, cpu);

	write_mmu_entryhi(cpu_context(cpu, next));
	TLBMISS_HANDLER_SETUP_PGD(next->pgd);

	/* mark mmu ownership change */
	cpumask_clear_cpu(cpu, mm_cpumask(prev));
	cpumask_set_cpu(cpu, mm_cpumask(next));

	local_irq_restore(flags);
#endif
}
#define deactivate_mm(tsk,mm)	do {} while (0)

/*
 * If mm is currently active_mm, we can't really drop it. Instead,
 * we will get a new one for it.
 */
#ifdef CONFIG_MMU
static inline void
drop_mmu_context(struct mm_struct *mm, unsigned cpu)
{
	unsigned long flags;

	local_irq_save(flags);

	if (cpumask_test_cpu(cpu, mm_cpumask(mm)))  {
		get_new_mmu_context(mm, cpu);
		write_mmu_entryhi(cpu_asid(cpu, mm));
	} else {
		/* will get a new context next time */
		cpu_context(cpu, mm) = 0;
	}

	local_irq_restore(flags);
}
#endif

#endif /* __ASM_CSKY_MMU_CONTEXT_H */
