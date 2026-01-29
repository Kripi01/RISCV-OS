#include "cpu.h"
#include "interrupt.h"
#include "process.h"
#include "screen.h"
#include "uart.h"
#include <stdint.h>
#include <stdio.h>

/* ======================================================= */
/* ==================== CONSOLE_PUTBYTES ================= */
/* ======================================================= */

void console_putbytes(const char *s, int len) {
  for (int i = 0; i < len; i++) {
    traite_car_uart(s[i]);
    traite_car(s[i]);
  }
}

/* ======================================================= */
/* ===================== UTILITAIRES ===================== */
/* ======================================================= */

// Affiche le drapeau français sur tout l'écran
void display_french_flag() {
  for (int y = 0; y < DISPLAY_HEIGHT; y++) {
    for (int x = 0; x < DISPLAY_WIDTH; x++) {
      if (x < DISPLAY_WIDTH / 3) {
        pixel(x, y, BLUE);
      } else if (x < 2 * DISPLAY_WIDTH / 3) {
        pixel(x, y, WHITE);
      } else {
        pixel(x, y, RED);
      }
    }
  }
}

void custom_sleep() {
  for (volatile int i = 0; i < 100000; i++) {
    // timeout
  }
}

void idle() {
  for (;;) {
    printf("[%s] pid = %li\n", mon_nom(), mon_pid());
    ordonnance();
  }
}

void proc1() {
  for (;;) {
    printf("[%s] pid = %li\n", mon_nom(), mon_pid());
    ordonnance();
  }
}

void kernel_start() {
  init_uart();
  init_ecran();

  init_traitant(mon_traitant);
  // NOTE: désactiver les interruptions au global ne suffit pas, il faut aussi
  // désactiver le timer car sinon, le CPU va appeler trap_handler (qui va
  // refuser l'IRQ car masquée) mais cela réveille le CPU du hlt de proc1
  // enable_timer();
  // enable_it();

  init_proc();
  cree_processus(proc1, "proc1");

  idle();

  /* on ne doit jamais sortir de kernel_start */
  while (1) {
    /* cette fonction arrete le processeur */
    hlt();
  }
}
