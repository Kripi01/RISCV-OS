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
	.text : {
		*(.text.init.test) *(.text.init) *(.text) *(.text.*)
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
		PROVIDE(_heap_start = __bss_end + 8);
		PROVIDE(_heap_end = ORIGIN(ram) + LENGTH(ram));
	} > ram
	_end = .;
}
