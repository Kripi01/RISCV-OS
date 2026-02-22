/*
 * ld directives the for kernel
 */
OUTPUT_ARCH( "riscv" )

MEMORY
{
	ram (wxa!ri) : ORIGIN = 0x80000000, LENGTH = 128M
}

SECTIONS {
	/* The kernel starts at 0x80000000 */
	. = 0x80000000;
  PROVIDE(_kernel_start = .); /* _kernel_start est bien aligné sur 4KB (=1 frame) */
	.text : {
		*(.text.init)
    /* Bricolage: on place les tests avant le buddy heap allocator pour éviter le BUG
     * (auipc) (cf. tests.c) */
    tests.o(.text .text.*)
    shell.o(.text .text.*)
    buddy_heap.o(.text .text.*)

    *(.text) *(.text.*)
		_etext = .;
	}  > ram

	.data : {
		*(.sdata) *(.sdata.*)
		*(.fini)
		*(.anno)
		*(.rodata) *(.rodata.*)
		*(__ex_table)
		*(.data) *(.data.*)
		_edata  =  .;
	}  > ram

	.bss : {
		/* On veut un alignement sur 8 octets */
		. = ALIGN(8);
		__bss_start = .;
		*(.sbss) *(.sbss.*)
		*(.bss) *(.bss.*)
		*(scommon) *(COMMON)
		. = ALIGN(8);
		__bss_end = .;
	} > ram

  . = ALIGN(4096);
  PROVIDE(_kernel_end = .); /* _kernel_end est aligné sur 4KB */

  . = ALIGN(4096); /* La mémoire libre doit être alignée sur 4KB */
  PROVIDE(_free_memory_start = .);
  PROVIDE(_memory_end = ORIGIN(ram) + LENGTH(ram));
	_end = .;
}
