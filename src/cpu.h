#ifndef __CPU_H__
#define __CPU_H__
#include "platform.h"
/* Met le processeur en pause en attente d'une interruption */
__inline__ static void hlt() { __asm__ __volatile__("wfi" ::: "memory"); }

/* Autorise les interruptions au niveau du processeur */
__inline__ static void enable_it() {
  __asm__("csrs mstatus, %0" ::"i"(MSTATUS_MIE));
}

/* Interdit les interruptions au niveau du processeur */
__inline__ static void disable_it() {
  __asm__("csrc mstatus, %0" ::"r"(MSTATUS_MIE));
}

#endif
