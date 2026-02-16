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
	} > ram

  . = ALIGN(4096);
  PROVIDE(_kernel_end = .); /* _kernel_end est aligné sur 4KB */

  /* Le tas est inclu dans les frames physiques */
  PROVIDE(_heap_start = .);
  PROVIDE(_heap_end = _heap_start + 32M); /* Tas de 32MB */
  . = . + 32M; /* on avance le curseur (PROVIDE ne l'avance pas) */

  . = ALIGN(4096); /* La mémoire libre doit être alignée sur 4KB */
  PROVIDE(_free_memory_start = .);
  PROVIDE(_memory_end = ORIGIN(ram) + LENGTH(ram));
	_end = .;
}
