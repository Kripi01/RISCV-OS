#ifndef __INTERRUPT_H__
#define __INTERRUPT_H__

#include "platform.h"

#define IT_FREQ 20
#define IT_TICS_REMAINING TIMER_FREQ / IT_FREQ

extern void mon_traitant();

typedef struct {
  uint64_t ra;      // 0(sp)
  uint64_t t0;      // 8(sp)
  uint64_t t1;      // 16(sp)
  uint64_t t2;      // 24(sp)
  uint64_t t3;      // 32(sp)
  uint64_t t4;      // 40(sp)
  uint64_t t5;      // 48(sp)
  uint64_t t6;      // 56(sp)
  uint64_t a0;      // 64(sp)
  uint64_t a1;      // 72(sp)
  uint64_t a2;      // 80(sp)
  uint64_t a3;      // 88(sp)
  uint64_t a4;      // 96(sp)
  uint64_t a5;      // 104(sp)
  uint64_t a6;      // 112(sp)
  uint64_t a7;      // 120(sp)
  uint64_t sp_user; // 128(sp)
} trap_ctxt_t;

uint32_t nbr_secondes();
void s_trap_handler(uint64_t scause, uint64_t sie, uint64_t sip,
                    trap_ctxt_t *tf);
void m_trap_handler(uint64_t mcause, uint64_t mie, uint64_t mip);
void init_traitant(void traitant());
void enable_timer();

#endif
