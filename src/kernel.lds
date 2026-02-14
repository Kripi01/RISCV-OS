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
		*(.text.init) *(.text) *(.text.*)
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

    /* TODO: Le tas doit être construit sur des frames de 4KB, donc inclus dans les frames physiques!!!! */
		PROVIDE(_heap_start = __bss_end + 8);
		PROVIDE(_heap_end = _heap_start + 32M); /* Tas de 32MB */

    . = ALIGN(4096); /* La mémoire libre doit être alignée sur 4KB (=1 frame) */
    PROVIDE(_free_memory_start = _heap_end);
    PROVIDE(_memory_end = ORIGIN(ram) + LENGTH(ram));
	} > ram
	_end = .;
}
