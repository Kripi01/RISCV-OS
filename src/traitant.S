# DOCS: The RISC-V Instruction Set Manual: Volume II: Privileged Architecture. 12.1.6. Supervisor Scratch (sscratch) Register

# Traitant d'interruption superivseur (stvec), donc exécuté en mode S.
    .globl mon_traitant
    .text
mon_traitant:
  # On passe à la pile kernel SSI on était en mode user (sinon, en mode S, on est déjà en pile kernel)

  # Si interruption en mode U: on swap sp (user) avec sscratch (kernel)
  # Si interruption en mode S: sscratch vaut 0, sp reste inchangé (c'est déjà le stack pointer du kernel)
  csrrw sp, sscratch, sp
  bnez sp, sauvegarde_contexte

  # On venait du mode S, donc on restaure le sp original
  csrrw sp, sscratch, sp

  # On ne change pas satp, donc l'interruption est traîtée avec la page table du mode U
  # mais en mode S. Ce n'est pas un problème car les adresses du kernel sont mappées en
  # 1:1 dans la page table du mode U. De plus, le mode S ne peut pas accèder aux
  # adresses user car elles ont le bit U et le SUM est désactivé (TODO:)
sauvegarde_contexte:
  addi sp, sp, -136
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

  # On sauvegarde le sp user du proc A sur la pile kernel du proc A
  csrr t0, sscratch
  sd t0, 128(sp) # si on était en mode S, alors 128(sp) contient zero

mon_traitant_fin_prologue:
  csrr a0, scause
  csrr a1, sie
  csrr a2, sip
  jal s_trap_handler

mon_traitant_debut_epilogue:
  ld ra, 0(sp)
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

  ld t0, 128(sp) # On récupère le sp user du proc B qui a été stocké sur la pile kernel du proc B (=sp suite au ctx_sw)
  beqz t0, retour_vers_kernel # Si le sp user est 0 alors le proc B était en mode S

retour_vers_user:
  csrw sscratch, t0 # on met le sp user dans sscratch (pour swap ensuite)
  ld t0, 8(sp) # on restaure enfin le vrai t0
  addi sp, sp, 136 # On libère la pile KERNEL du proc B
  csrrw sp, sscratch, sp # On passe au sp user du proc B
  sret # sret repasse au mode user (sstatus.SPP), le satp est celui du mode U, donc on
       # peut de nouveau accèder aux adresses virtuelles (sepc, ra et sp du mode U:)


retour_vers_kernel:
  # On ne swappe pas sscratch et sp car sscratch == 0
  ld t0, 8(sp)
  addi sp, sp, 136
  sret

# NOTE: s'il n'y a pas de changement de contexte durant l'interruption e.g. interruption UART alors A == B et ça marche quand même.
