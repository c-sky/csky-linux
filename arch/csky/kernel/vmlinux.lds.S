#include <asm/vmlinux.lds.h>
#include <asm/page.h>

OUTPUT_ARCH(csky)
ENTRY(_start)

#ifndef __cskyBE__ 
jiffies = jiffies_64;
#else
jiffies = jiffies_64 + 4;
#endif

SECTIONS
{
	. = PAGE_OFFSET + (CONFIG_RAM_BASE - PHYS_OFFSET);
	_text = .;
	_stext = .;
	.text : AT(ADDR(.text) - LOAD_OFFSET) {
		TEXT_TEXT
		SCHED_TEXT
		LOCK_TEXT
		IRQENTRY_TEXT
		SOFTIRQENTRY_TEXT
		KPROBES_TEXT
	} = 0
	_etext = .;
	EXCEPTION_TABLE(1024)

	_sdata = .;
	_data = .;
	RO_DATA_SECTION(PAGE_SIZE)
	RW_DATA_SECTION(0x20, PAGE_SIZE, THREAD_SIZE)
	_edata = .;

	__init_begin = .;
	HEAD_TEXT_SECTION  
	INIT_TEXT_SECTION(PAGE_SIZE)
	INIT_DATA_SECTION(PAGE_SIZE)
	PERCPU_SECTION(L1_CACHE_BYTES)
	__init_end = .;

	BSS_SECTION(0, 0, 0)
	. = ALIGN(PAGE_SIZE);
	swapper_pg_dir = .;
	. += PAGE_SIZE;
	invalid_pte_table = .;
	. += PAGE_SIZE;
	vec_base = .;
	. += 512;
	. = ALIGN(PAGE_SIZE);
	_end = . ;

	STABS_DEBUG
	DWARF_DEBUG
	NOTES

	DISCARDS
}
