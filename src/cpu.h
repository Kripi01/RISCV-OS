#ifndef __CPU_H__
#define __CPU_H__
#include "platform.h"
// Met le processeur en pause en attente d'une interruption
__inline__ static void hlt() { __asm__ __volatile__("wfi" ::: "memory"); }

// Autorise les interruptions au niveau du processeur (en mode machine)
__inline__ static void m_enable_it() {
  __asm__("csrs mstatus, %0" ::"i"(MSTATUS_MIE));
}

// Interdit les interruptions au niveau du processeur (en mode machine)
__inline__ static void m_disable_it() {
  __asm__("csrc mstatus, %0" ::"i"(MSTATUS_MIE));
}

// Autorise les interruptions au niveau du processeur (en mode superviseur)
__inline__ static void s_enable_it() {
  __asm__("csrs sstatus, %0" ::"i"(SSTATUS_SIE));
}

// Interdit les interruptions au niveau du processeur (en mode superviseur)
__inline__ static void s_disable_it() {
  __asm__("csrc sstatus, %0" ::"i"(SSTATUS_SIE));
}

__inline__ static void enable_sum() {
  __asm__("li t0, 0x40000\n"
          "csrs sstatus, t0");
}
__inline__ static void disable_sum() {
  __asm__("li t0, 0x40000\n"
          "csrc sstatus, t0");
}

// Modifie mepc pour pouvoir revenir à l'instruction suivant le ecall.
__inline__ static void return_ecall_m() {
  // mepc stocke le pc de ecall, donc il faut l'incrémenter de la taille d'une
  // instruction = 32 bits
  uint64_t mepc;
  __asm__("csrr %0, mepc" : "=r"(mepc));
  mepc += 4;
  __asm__("csrw mepc, %0" ::"r"(mepc));
}

// Modifie sepc pour pouvoir revenir à l'instruction suivant le ecall.
__inline__ static void return_ecall_s() {
  // sepc stocke le pc de ecall, donc il faut l'incrémenter de la taille d'une
  // instruction = 32 bits
  uint64_t sepc;
  __asm__("csrr %0, sepc" : "=r"(sepc));
  sepc += 4;
  __asm__("csrw sepc, %0" ::"r"(sepc));
}

#endif
