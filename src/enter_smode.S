# DOCS:
# mideleg/medeleg: https://five-embeddev.com/riscv-priv-isa-manual/Priv-v1.12/machine.html#machine-trap-delegation-registers-medeleg-and-mideleg

# pmp (physical memory protection): https://five-embeddev.com/riscv-priv-isa-manual/Priv-v1.12/machine.html#sec:pmp
# https://riscv.org/blog/adding-physical-memory-protection-to-the-veer-el2-risc-v-core-2/ (partie Principles of RISC-V PMP)

# CLINT:
# https://static.dev.sifive.com/FE310-G000.pdf (Chapitre 11 et plus
# particulièrement, 11.3)

    .text
    .globl enter_smode
enter_smode:
    # On passe au mode supervisor (pour l'instant, on est en mode machine).
    # Pour cela, on simule un retour d'interruption.
    # NOTE: on est obligé d'initialiser les registres du mode M (mtvec, les registres
    # pmp, mstatus, etc) maintenant car après, en mode S, on ne peut plus les modifier

    li t0, 0x1800
    csrc mstatus, t0 # on clear mstatus.MPP
    li t1, 0x800
    csrs mstatus, t1 # mstatus.MPP = 01 = S-mode (on était en mode M = 11)

    # Il faut donner les permissions RWX à toute la mémoire au mode S, via le PMP
    li t0, 0xFFFFFFFF
    csrw pmpaddr0, t0
    li t1, 0x0000001F
    csrw pmpcfg0, t1

    li t0, 0x222
    csrs sie, t0 # on autorise toutes les interruptions pour le mode S
    li t1, 0x888
    csrs mie, t1 # on autorise toutes les interruptions pour le mode M
    csrw mideleg, t0 # on délègue les IRQ soft, timer et externes au mode S

    # On délègue les exceptions de page faults (12, 13, 15) et de ecall du mode U (8)
    # vers le mode S [ Docs: medeleg has a bit position allocated for every synchronous
    # exception with the index of the bit position equal to the value returned in the
    # mcause(=scause) register ]
    li t0, 0xb100
    csrw medeleg, t0

    # Les autres exceptions restent gérées par le mode M (update timer, cf. interrupt.c)
    la t0, mon_traitant_m
    csrw mtvec, t0

    # on active le bit sstatus.SPIE pour que après le mret, le bit mstatus.SIE soit à 1
    li t0, 0x20
    csrs mstatus, t0

    # On commence les choses sérieuses en appelant le code du noyau
    la t0, kernel_start
    csrw mepc, t0
    mret # l'appel de mret va "remettre" le mode à mstatus.MPP, donc au mode S
