#include <linux/init.h>
#include <linux/sched.h>
#include <linux/mm.h>

#include <asm/setup.h>
#include <asm/mmu_context.h>
#include <linux/module.h>
#include <asm/pgtable.h>
#include <abi/ckmmu.h>

void local_flush_tlb_all(void)
{
	tlb_invalid_all();
}

void local_flush_tlb_mm(struct mm_struct *mm)
{
	int cpu = smp_processor_id();

	if (cpu_context(cpu, mm) != 0)
		drop_mmu_context(mm,cpu);
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
				if (idx >= 0)
					tlb_invalid_indexed();
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

        local_irq_save(flags);
        size = (end - start + (PAGE_SIZE - 1)) >> PAGE_SHIFT;
        if (size <= CSKY_TLB_SIZE) {
                int oldpid = read_mmu_entryhi();

                start &= PAGE_MASK;
                end += PAGE_SIZE - 1;
                end &= PAGE_MASK;

                while (start < end) {
                        int idx;
                        write_mmu_entryhi(start);
                        start += PAGE_SIZE;             /* BARRIER */
                        tlb_probe();
                        idx = read_mmu_index();
                        if (idx >= 0)
                                tlb_invalid_indexed();
                }
                write_mmu_entryhi(oldpid);
        } else {
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

		newpid = cpu_asid(cpu, vma->vm_mm);
		page &= (PAGE_MASK << 1);
		local_irq_save(flags);
		oldpid = read_mmu_entryhi();
		write_mmu_entryhi(page | newpid);
		tlb_probe();
		idx = read_mmu_index();
		if(idx >= 0)
			tlb_invalid_indexed();
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
	if (idx >= 0)
		tlb_invalid_indexed();
	write_mmu_entryhi(oldpid);
	local_irq_restore(flags);
}

EXPORT_SYMBOL(local_flush_tlb_one);

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
		entrylo0 = entrylo0;
		entrylo1 = read_mmu_entrylo1();
		entrylo1 = entrylo1;
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
