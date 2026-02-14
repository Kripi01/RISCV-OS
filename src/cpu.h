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

#endif
