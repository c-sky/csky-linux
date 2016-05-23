#include <asm-generic/vmlinux.lds.h>
#include <asm/page.h>

OUTPUT_ARCH(csky) 

ENTRY(__LMA_START)
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
  _stext = .;			/* Text and read-only data */
  _text = .;			/* Text and read-only data */
  __LMA_START = _start ;
  .text : AT(ADDR(.text) ) {
	HEAD_TEXT
	TEXT_TEXT
	SCHED_TEXT
	LOCK_TEXT
	*(.fixup)
	*(.gnu.warning)
	} :text = 0

  _etext = .;			/* End of text section */

  . = ALIGN(16);		/* Exception table */
  __start___ex_table = .;
  __ex_table : { *(__ex_table) }
  __stop___ex_table = .;

  RODATA

	.data : AT(ADDR(.data) )
	{
		_data = .;
	        _sdata = .;
		DATA_DATA
		CONSTRUCTORS
		_edata = .;
		. = ALIGN(PAGE_SIZE);
		swapper_pg_dir = .;
		. += PAGE_SIZE;
		invalid_pte_table = .;
		. += PAGE_SIZE;
		vec_base = .;
		. += 512;
	}

  . = ALIGN(16);
  .data.cacheline_aligned : { *(.data.cacheline_aligned) }

  .bss : AT(ADDR(.bss) ) {
	VMLINUX_SYMBOL(__bss_start) = .;	
	_sbss = . ; 
	 *(.bss) 
	_ebss = . ;
	VMLINUX_SYMBOL(__bss_stop) = .;
	}		/* BSS */

  _edata = .;			/* End of data section */

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

  .data..init_task : AT(ADDR(.data..init_task) ) { *(.data..init_task) }	/* The initial task and kernel stack */

  _end = . ;

  /* Sections to be discarded */
  /DISCARD/ : {
	EXIT_TEXT
	EXIT_DATA
	*(.exitcall.exit)
	}

  /* Stabs debugging sections.  */
	STABS_DEBUG
        DWARF_DEBUG

        /* These must appear regardless of  .  */
        .gptab.sdata : {
                *(.gptab.data)
                *(.gptab.sdata)
        }
        .gptab.sbss : {
                *(.gptab.bss)
                *(.gptab.sbss)
        }

}
