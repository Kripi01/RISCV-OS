    .equ STACK_SIZE,   4096

    .section .text.init,"ax",@progbits
    .globl entry,init_vga,console_putbytes,__console_putbytes__

entry:
    # Mise en place d'un pointeur de pile pour pouvoir appeler
    # quelques fonctions en mode machine
    la sp, stacks + STACK_SIZE

    # Raz de bss par paquets de 8 octets
    la t0, __bss_start
    la t1, __bss_end
1:  sd zero, 0(t0)
    addi t0, t0, 8
    bleu t0, t1, 1b

    # NOTE: Le passage au mode superviseur appelle la fonction kernel_start
    j enter_smode

    .weak console_putbytes,__console_putbytes__
__console_putbytes__:
    ret
console_putbytes:
    j __console_putbytes__
    ret

    .bss
    .align 8
    .global stacks
stacks:
    .skip STACK_SIZE
