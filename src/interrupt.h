#ifndef __INTERRUPT_H__
#define __INTERRUPT_H__

#include "platform.h"

#define IT_FREQ 20
#define IT_TICS_REMAINING TIMER_FREQ / IT_FREQ

extern void mon_traitant();

uint32_t nbr_secondes();
void trap_handler(uint64_t mcause, uint64_t mie, uint64_t mip);
void init_traitant(void traitant());
void enable_timer();

#endif
