#include <linux/init.h>
#include <linux/sched.h>
#include <linux/mm.h>

#include <asm/setup.h>
#include <asm/mmu_context.h>
#include <linux/module.h>
#include <asm/pgtable.h>

#include <hal/ckmmu.h>

#undef DEBUG_TLB
#undef DEBUG_TLBUPDATE

#define ENTER_CRITICAL(flags) local_irq_save(flags)
#define EXIT_CRITICAL(flags) local_irq_restore(flags)

void local_flush_tlb_all(void)
{
	unsigned long flags;
	unsigned long old_ctx;
	int entry;

#ifdef DEBUG_TLB
	printk("[tlball]");
#endif

	local_irq_save(flags);
	/* Save old context and create impossible VPN2 value */
	old_ctx = read_mmu_entryhi();
	write_mmu_entrylo0(0);
	write_mmu_entrylo1(0);

	entry = read_mmu_wired();

	/* Blast 'em all away. */
	while (entry < CSKY_TLB_SIZE) {
		/*
		 * Make sure all entries differ.  If they're not different
		 * CSKY will take revenge ...
		 */
		write_mmu_entryhi(KSEG0 + entry * 0x2000);
		write_mmu_index(entry);
		tlb_write_indexed();
		entry++;
	}
	write_mmu_entryhi(old_ctx);
	local_irq_restore(flags);
}

void local_flush_tlb_mm(struct mm_struct *mm)
{
	int cpu = smp_processor_id();

	if (cpu_context(cpu, mm) != 0) {
#ifdef DEBUG_TLB
		printk("[tlbmm<%d>]", cpu_context(cpu, mm));
#endif
		drop_mmu_context(mm,cpu);
	}
}

void local_flush_tlb_range(struct vm_area_struct *vma, unsigned long start,
        unsigned long end)
{
	struct mm_struct *mm = vma->vm_mm;
	int cpu = smp_processor_id();

	if (cpu_context(cpu, mm) != 0) {
		unsigned long size, flags;

		local_irq_save(flags);
		size = (end - start + (PAGE_SIZE - 1)) >> PAGE_SHIFT;
		size = (size + 1) >> 1;
		if (size <= CSKY_TLB_SIZE/2) {
			int oldpid = read_mmu_entryhi();
			int newpid = cpu_asid(cpu, mm);

			start &= (PAGE_MASK << 1);
			end += ((PAGE_SIZE << 1) - 1);
			end &= (PAGE_MASK << 1);
			while (start < end) {
				int idx;

				write_mmu_entryhi(start | newpid);
				start += (PAGE_SIZE << 1);
				tlb_probe();
				idx = read_mmu_index();
				write_mmu_entrylo0(0);
				write_mmu_entrylo1(0);
				if (idx < 0)
				{
					write_mmu_entryhi((start | newpid) + 1);
					continue;
				}
				/* Make sure all entries differ. */
				write_mmu_entryhi(KSEG0 + idx*0x2000);
				tlb_write_indexed();
			}
			write_mmu_entryhi(oldpid);
		} else {
			drop_mmu_context(mm, cpu);
		}
		local_irq_restore(flags);
	}
}

void local_flush_tlb_kernel_range(unsigned long start, unsigned long end)
{
	unsigned long size, flags;

#ifdef DEBUG_TLB
	printk("[tlbrange<0x%08lx,0x%08lx>]\n", start, end);
#endif
	local_irq_save(flags);
	size = (end - start + (PAGE_SIZE - 1)) >> PAGE_SHIFT;
	size = (size + 1) >> 1;
	if (size <= CSKY_TLB_SIZE / 2) {
		int idx;
		unsigned int page;
		int pid = read_mmu_entryhi();

		start &= (PAGE_MASK << 1);
		end += ((PAGE_SIZE << 1) - 1);
		end &= (PAGE_MASK << 1);
		for (idx = 0; idx < CSKY_TLB_SIZE; idx++) {
			write_mmu_index(idx);
			tlb_read();
			page = read_mmu_entryhi();
			page &=(PAGE_MASK << 1);
			if (page >= start && page < end) {
				write_mmu_entrylo0(0);
				write_mmu_entrylo1(0);
				write_mmu_entryhi(KSEG0 + idx * 0x2000);
				tlb_write_indexed();
			}
		}
		write_mmu_entryhi(pid);
	}
	else {
		local_flush_tlb_all();
	}
	local_irq_restore(flags);
}

void local_flush_tlb_page(struct vm_area_struct *vma, unsigned long page)
{
	int cpu = smp_processor_id();

	if (!vma || cpu_context(cpu, vma->vm_mm) != 0) {
		unsigned long flags;
		int oldpid, newpid, idx;

#ifdef DEBUG_TLB
		printk("[tlbpage<%d,%08lx>]", cpu_context(cpu, vma->vm_mm),
		       page);
#endif
		newpid = cpu_asid(cpu, vma->vm_mm);
		page &= (PAGE_MASK << 1);
		local_irq_save(flags);
		oldpid = read_mmu_entryhi();
		write_mmu_entryhi(page | newpid);
		tlb_probe();
		idx = read_mmu_index();
		write_mmu_entrylo0(0);
		write_mmu_entrylo1(0);
		if(idx < 0)
		{
			write_mmu_entryhi((page | newpid) + 1);
			goto finish;
		}
		/* Make sure all entries differ. */
		write_mmu_entryhi(KSEG0+idx*0x2000);
		tlb_write_indexed();

	finish:
		write_mmu_entryhi(oldpid);
		local_irq_restore(flags);
	}
}

/*
 * Remove one kernel space TLB entry.  This entry is assumed to be marked
 * global so we don't do the ASID thing.
 */
void local_flush_tlb_one(unsigned long page)
{
	unsigned long flags;
	int oldpid, idx;

	page &= (PAGE_MASK << 1);
	oldpid = read_mmu_entryhi();
	local_irq_save(flags);
	page = page | (oldpid & 0xff);
	write_mmu_entryhi(page);
	tlb_probe();
	idx = read_mmu_index();
	write_mmu_entrylo0(0);
	write_mmu_entrylo1(0);
	if (idx >= 0) {
		/* Make sure all entries differ. */
		write_mmu_entryhi(KSEG0+idx*0x2000);
		tlb_write_indexed();
	}
	else
		write_mmu_entryhi(page + 1);
	write_mmu_entryhi(oldpid);
	local_irq_restore(flags);
}

EXPORT_SYMBOL(local_flush_tlb_one);

void __update_tlb(struct vm_area_struct * vma, unsigned long address, pte_t pte)
{
	unsigned long flags;
	pgd_t *pgdp;
	pud_t *pudp;
	pmd_t *pmdp;
	pte_t *ptep;
	int idx, pid;

	/*
	 * Handle debugger faulting in for debugee.
	 */
	if (current->active_mm != vma->vm_mm)
		return;

	pid = read_mmu_entryhi() & ASID_MASK;

	local_irq_save(flags);
	address &= (PAGE_MASK << 1);
	write_mmu_entryhi(address | pid);
	pgdp = pgd_offset(vma->vm_mm, address);
	tlb_probe();
	pudp = pud_offset(pgdp, address);
	pmdp = pmd_offset(pudp, address);
	idx = read_mmu_index();
	ptep = pte_offset(pmdp, address);

	write_mmu_entrylo0(pte_val(*ptep++) >> 6);
	write_mmu_entrylo1(pte_val(*ptep) >> 6);
	write_mmu_entryhi(address | pid);
	if (idx < 0) {
		write_mmu_entryhi((address | pid) + 1);
		write_mmu_entryhi(address | pid);
		tlb_write_random();
	} else {
		tlb_write_indexed();
	}
	local_irq_restore(flags);
}

/* show current 32 jtlbs */
void show_jtlb_table(void)
{
	unsigned long flags;
	int entryhi, entrylo0, entrylo1;
	int entry;
	int oldpid;

	local_irq_save(flags);
	entry = 0;
	printk("\n\n\n");

	oldpid = read_mmu_entryhi();
	while (entry < CSKY_TLB_SIZE)
	{
		write_mmu_index(entry);
		tlb_read();
		entryhi = read_mmu_entryhi();
		entrylo0 = read_mmu_entrylo0();
		entrylo0 = entrylo0 << 6;
		entrylo1 = read_mmu_entrylo1();
		entrylo1 = entrylo1 << 6;
		printk("jtlb[%d]:	entryhi - 0x%x;	entrylo0 - 0x%x;"
		       "	entrylo1 - 0x%x\n",
			 entry, entryhi, entrylo0, entrylo1);
		entry++;
	}
	write_mmu_entryhi(oldpid);
	local_irq_restore(flags);
}

void __init csky_tlb_init(void)
{
	local_flush_tlb_all();
}
