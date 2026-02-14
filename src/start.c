#include "buddy_heap.h"
#include "cpu.h"
#include "interrupt.h"
#include "keyboard.h"
#include "process.h"
#include "screen.h"
#include "shell.h"
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
  _place_curseur(1, 0); // On affiche le premier curseur

  for (;;) {
    s_enable_it();
    hlt();
    s_disable_it();
  }
}

int proc1() {
  for (int32_t i = 0; i < 2; i++) {
    printf("[temps = %u] processus %s pid = %li\n", nbr_secondes(), mon_nom(),
           mon_pid());
    dors(2);
  }

  return 0;
}

int proc2() {
  for (int32_t i = 0; i < 2; i++) {
    printf("[temps = %u] processus %s pid = %li\n", nbr_secondes(), mon_nom(),
           mon_pid());
    dors(3);
  }
  cree_processus(proc1, "new_proc1");

  return 0;
}

int proc3() {
  for (int32_t i = 0; i < 2; i++) {
    printf("[temps = %u] processus %s pid = %li\n", nbr_secondes(), mon_nom(),
           mon_pid());
    dors(5);
  }
  cree_processus(proc2, "new_proc2");

  return 0;
}

int help() {
  printf("Commandes disponibles: help, ps, true, false, bash, clear\n");
  return 0;
}

int f_true() { return 0; }

int f_false() { return 1; }

int history() {
  printf("todo\n");
  return 0;
}

int clear() {
  printf("\f");
  return 0;
}

int fg() {
  printf("todo\n");
  return 0;
}

void kernel_start() {
  init_uart();
  enable_uart_interrupts();
  init_ecran();

  buddy_init_heap();

  init_traitant(mon_traitant); // pour le mode S
  enable_timer();

  init_proc(); // crée idle et l'élit.

  // WARNING: il ne faut pas faire créer le premier bash par idle (avec waitpid)
  // car sinon, idle serait en attente de la mort de bash et quand bash lance un
  // processus qui se met à dormir alors il y a interblocage.
  cree_processus(bash, "bash");

  idle();

  /* on ne doit jamais sortir de kernel_start */
  while (1) {
    /* cette fonction arrete le processeur */
    hlt();
  }
}
