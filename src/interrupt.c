#include "interrupt.h"
#include "keyboard.h"
#include "platform.h"
#include "process.h"
#include "screen.h"
#include <stdint.h>
#include <stdio.h>

static volatile uint32_t time = 0;

// Renvoie le nombre de secondes depuis le lancement du programme.
uint32_t nbr_secondes() {
  // return *(uint64_t *)(CLINT_TIMER) / TIMER_FREQ;
  return time / IT_FREQ;
}

// Gère (ou masque) les interruptions.
// WARNING: Ne gère pas les exceptions, le bit 63 de mcause est ignoré!!
void trap_handler(uint64_t mcause, uint64_t mie, uint64_t mip) {
  uint64_t masked_mcause = mcause & 0x7FFFFFFFFFFFFFFF;
  if ((mie & mip) == 0) { // Interruption masquée
    return;
  }

  if (masked_mcause == 3) { // Interruption software
    // TODO: Interruptions softwares
  } else if (masked_mcause == 7) { // Interruption timer
    time++;

    // On affiche le temps depuis le démarrage en haut à droite
    uint32_t n = nbr_secondes();
    char time_str[18];
    sprintf(time_str, "[%02u:%02u:%02u]", (n / 3600) % 100, (n / 60) % 60,
            n % 60);
    display_top_right(time_str, 10);

    // On acquitte l'IRQ et on réenclenche la prochaine interruption du timer.
    *(volatile uint64_t *)(CLINT_TIMER_CMP) =
        *(volatile uint64_t *)CLINT_TIMER + IT_TICS_REMAINING;

    // On change de processus à chaque interruption timer.
    ordonnance();
  } else if (masked_mcause == 11) { // Interruption externe
    // DOC: A read of zero indicates that no interrupts are pending. A non-zero
    // read contains the id of the highest pending interrupt. A write to this
    // register signals completion of the interrupt id writte

    // On claim l'interruption en lisant le registre claim/complete.
    uint32_t interrupt_id = *(volatile uint32_t *)PLIC_IRQ_CLAIM;
    if (interrupt_id == 10) {
      // On gère l'interruption de l'UART.
      int c = getchar_uart();
      printf("%c", c);

      // On complète l'interruption en écrivant l'ID de l'interrupt dans le
      // regsitre claim/complete.
      *(volatile uint32_t *)PLIC_IRQ_CLAIM = interrupt_id;
    }
  }
}

// Initialise le vecteur d'interruption.
void init_traitant(void traitant()) {
  __asm__("csrw mtvec, %0" ::"r"(traitant));
}

// Autorise les interruptions du timer (et initialise la première)
void enable_timer() {
  // On démasque l'interruption du timer (et globale)
  __asm__("csrs mie, %0" ::"r"(1 << IRQ_M_TMR));

  // Envoi de la première interruption par le timer
  *(volatile uint64_t *)(CLINT_TIMER_CMP) =
      *(volatile uint64_t *)CLINT_TIMER + IT_TICS_REMAINING;
}
