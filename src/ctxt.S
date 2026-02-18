    .text
    .globl ctx_sw

/* Contexte :
 *   contexte1 : registre a0 # contexte1 est un tableau de 20 uint64_t
 *   contexte2 : registre a1 # contexte2 est un tableau de 20 uint64_t
 * */
ctx_sw:
ctx_sw_fin_prologue:
  # On sauvegarde dans le contexte du proc 1 les registres devant être laissés intacts
  # NOTE: Les autres registres (t0-t6, a0-a7) sont sauvegardés par traitant.S
  sd ra, 0(a0)
  sd sp, 8(a0)
  sd gp, 16(a0)
  sd tp, 24(a0)
  sd s0, 32(a0)
  sd s1, 40(a0)
  sd s2, 48(a0)
  sd s3, 56(a0)
  sd s4, 64(a0)
  sd s5, 72(a0)
  sd s6, 80(a0)
  sd s7, 88(a0)
  sd s8, 96(a0)
  sd s9, 104(a0)
  sd s10, 112(a0)
  sd s11, 120(a0)
  csrr t0, sepc
  sd t0, 128(a0)
  csrr t0, sstatus
  sd t0, 136(a0)
  csrr t0, satp
  sd t0, 144(a0)
  csrr t0, sscratch
  sd t0, 152(a0)

  # On restaure les mêmes registres mais correspondant au processus 2
  ld ra, 0(a1) # Ce ra est mappé avec la page table du processus 2
  ld sp, 8(a1)
  ld gp, 16(a1)
  ld tp, 24(a1)
  ld s0, 32(a1)
  ld s1, 40(a1)
  ld s2, 48(a1)
  ld s3, 56(a1)
  ld s4, 64(a1)
  ld s5, 72(a1)
  ld s6, 80(a1)
  ld s7, 88(a1)
  ld s8, 96(a1)
  ld s9, 104(a1)
  ld s10, 112(a1)
  ld s11, 120(a1)
  ld t0, 128(a1)
  csrw sepc, t0
  ld t0, 136(a1)
  csrw sstatus, t0 # Le mode précédent sera donc rétabli (S si idle et U sinon)
  ld t0, 144(a1)
  csrw satp, t0
  ld t0, 152(a1)
  csrw sscratch, t0

  # on flushe le TLB pour que l'écriture de satp soit prise en compte dans le buffer
  sfence.vma zero, zero

  # On jump vers ra. Au premier context switch du processus, on jump vers init_proc_launcher
  # qui va appeler proc_launcher (qui passe en mode U) avec le code du processus
  # (contenu dans s0, cf. processs.c) i.e. vers proc_launcher avec les bons arguments
  ret

ctx_sw_debut_epilogue:
