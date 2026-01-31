    .text
    .globl ctx_sw

/* Contexte :
 *   contexte1 : registre a0 # contexte1 est un tableau de 18 uint64_t
 *   contexte2 : registre a1 # contexte2 est un tableau de 18 uint64_t
 * */
ctx_sw:
ctx_sw_fin_prologue:
  # On sauvegarde dans la zone de contexte du processus 1 les registres 
  # devant être laissés intacts
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
  csrr t0, mepc
  sd t0, 128(a0)
  csrr t0, mstatus
  sd t0, 136(a0)

  # On restaure les mêmes registres mais correspondant au processus 2
  ld ra, 0(a1)
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
  csrw mepc, t0
  ld t0, 136(a1)
  csrw mstatus, t0

  # Enfin, on donne en argument à proc_launcher (l'adresse de ra) la valeur de la
  # fonction du processus 2, qui est la première valeur de la pile (cf cree_processus)
  ld a0, 0(sp)
  ret # Puis on jump vers ra, i.e. vers proc_launcher avec les bons arguments

ctx_sw_debut_epilogue:
