# Traitant d'interruption machine (mtvec), donc exécuté en mode M.
    .globl mon_traitant_m
    .text
mon_traitant_m:
  # Si on vient du mode U/S alors sp n'est pas fiable (car provient d'un mode
  # inférieur) donc on utilise une pile propre au mode machine: mscratch (cf.
  # crt0.S pour l'initialisation de mscratch).
  # De plus si on vient du mode U, sp est une adresse virtuelle (e.g. 0x50000000 qui ne
  # correspond à rien en adresse physique).
  csrrw sp, mscratch, sp

  addi sp, sp, -128
  sd ra, 0(sp)
  sd t0, 8(sp)
  sd t1, 16(sp)
  sd t2, 24(sp)
  sd t3, 32(sp)
  sd t4, 40(sp)
  sd t5, 48(sp)
  sd t6, 56(sp)
  sd a0, 64(sp)
  sd a1, 72(sp)
  sd a2, 80(sp)
  sd a3, 88(sp)
  sd a4, 96(sp)
  sd a5, 104(sp)
  sd a6, 112(sp)
  sd a7, 120(sp)

mon_traitant_m_fin_prologue:
  csrr a0, mcause
  csrr a1, mie
  csrr a2, mip
  jal m_trap_handler

mon_traitant_m_debut_epilogue:
  ld ra, 0(sp)
  ld t0, 8(sp)
  ld t1, 16(sp)
  ld t2, 24(sp)
  ld t3, 32(sp)
  ld t4, 40(sp)
  ld t5, 48(sp)
  ld t6, 56(sp)
  ld a0, 64(sp)
  ld a1, 72(sp)
  ld a2, 80(sp)
  ld a3, 88(sp)
  ld a4, 96(sp)
  ld a5, 104(sp)
  ld a6, 112(sp)
  ld a7, 120(sp)
  addi sp, sp, 128

  csrrw sp, mscratch, sp # on rétablit le sp user/supervisor

  mret
