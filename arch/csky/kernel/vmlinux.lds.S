#include <asm/vmlinux.lds.h>
#include <asm/page.h>

OUTPUT_ARCH(csky)
ENTRY(_start)
PHDRS {
	text PT_LOAD FLAGS(7);  /* RWX */
	data PT_LOAD FLAGS(7);
	note PT_NOTE FLAGS(4);  /* R__ */
}

#ifndef __cskyBE__ 
jiffies = jiffies_64;
#else
jiffies = jiffies_64 + 4;
#endif

SECTIONS
{
	. = PAGE_OFFSET + (CONFIG_RAM_BASE - PHYS_OFFSET);
	_start = .;			/* Text and read-only data */
  
	.text : AT(ADDR(.text) ) {
		_text = .;
		_stext = .;
		HEAD_TEXT
		TEXT_TEXT
		SCHED_TEXT
		LOCK_TEXT
		*(.fixup)
		*(.gnu.warning)
		_etext = .;
	} :text = 0

	EXCEPTION_TABLE(0x10)

	_sdata = .;
	_data = .;
	RO_DATA_SECTION(PAGE_SIZE)
	RW_DATA_SECTION(0x20, PAGE_SIZE, THREAD_SIZE)
	_edata = .;

		. = ALIGN(PAGE_SIZE);
		swapper_pg_dir = .;
		. += PAGE_SIZE;
		invalid_pte_table = .;
		. += PAGE_SIZE;
		vec_base = .;
		. += 512;


	BSS_SECTION(0x20, 0, 0x20)

  /* will be freed after init */
  . = ALIGN(4096);		/* Init code and data */
  __init_begin = .;
  .init.text : AT(ADDR(.init.text) ) {
	_sinittext = .;
	INIT_TEXT
	_einittext = .;
	__tagtable_begin = .;
	*(.taglist.init)
	__tagtable_end = .;  
  } :data
  .init.data : AT(ADDR(.init.data) ) { INIT_DATA }
  . = ALIGN(16);
  __setup_start = .;
  .init.setup : AT(ADDR(.init.setup) ) { *(.init.setup) }
  __setup_end = .;
  __initcall_start = .;
  .initcall.init : AT(ADDR(.initcall.init) ) {
	INIT_CALLS
  }
  __initcall_end = .;
  __con_initcall_start = .;
  .con_initcall.init : AT(ADDR(.con_initcall.init) ) { *(.con_initcall.init) }
  __con_initcall_end = .;
  .csky_fixup : AT(ADDR(.csky_fixup) ) {
	__start_fixup = .;
	*(.csky_fixup)
	__stop_fixup = .;
  }
  SECURITY_INIT
#ifdef CONFIG_BLK_DEV_INITRD
  . = ALIGN(8192);
  __initramfs_start = .;
  .init.ramfs : AT(ADDR(.init.ramfs) ) { *(.init.ramfs) }
  __initramfs_end = .;
#endif
  NOTES
  . = ALIGN(8192);
  __init_end = .;

	_end = . ;

	STABS_DEBUG
	DWARF_DEBUG

	DISCARDS
}
