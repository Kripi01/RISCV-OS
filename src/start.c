#include "cpu.h"
#include "interrupt.h"
#include "keyboard.h"
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

void idle() {
  for (;;) {
    int c = getchar_uart();
    printf("%c", c);

    enable_it();
    hlt();
    disable_it();
  }
}

void proc1() {
  for (int32_t i = 0; i < 2; i++) {
    printf("[temps = %u] processus %s pid = %li\n", nbr_secondes(), mon_nom(),
           mon_pid());
    dors(2);
  }
}

void proc2() {
  for (int32_t i = 0; i < 2; i++) {
    printf("[temps = %u] processus %s pid = %li\n", nbr_secondes(), mon_nom(),
           mon_pid());
    dors(3);
  }
  cree_processus(proc1, "new_proc1");
}

void proc3() {
  for (int32_t i = 0; i < 2; i++) {
    printf("[temps = %u] processus %s pid = %li\n", nbr_secondes(), mon_nom(),
           mon_pid());
    dors(5);
  }
  cree_processus(proc2, "new_proc2");
}

void kernel_start() {
  init_uart();
  init_ecran();

  init_traitant(mon_traitant);
  enable_timer();

  init_proc();
  cree_processus(proc1, "proc1");
  // cree_processus(proc2, "proc2");
  // cree_processus(proc3, "proc3");

  idle();

  /* on ne doit jamais sortir de kernel_start */
  while (1) {
    /* cette fonction arrete le processeur */
    hlt();
  }
}
