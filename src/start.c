// TODO: Dans tous les fichiers, utiliser au maximum des static pour
// l'encapsulation

#include "buddy_heap.h"
#include "cpu.h"
#include "interrupt.h"
#include "keyboard.h"
#include "pm.h"
#include "process.h"
#include "screen.h"
#include "shell.h"
#include "uart.h"
#include <stdint.h>

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

// idle est le seul processus à être exécuté en mode S (pour les wfi,...)
void idle() {
  _place_curseur(1, 0); // On affiche le premier curseur

  for (;;) {
    s_enable_it();
    hlt();
    s_disable_it();
  }
}

void kernel_start() {
  init_traitant(mon_traitant); // traitant du mode S
  init_uart();
  enable_uart_interrupts();
  init_ecran();

  buddy_init_heap();
  init_frames();

  init_proc(); // crée idle et l'élit.
  // WARNING: il ne faut pas faire créer le premier bash par idle (avec waitpid)
  // car sinon, idle serait en attente de la mort de bash et quand bash lance un
  // processus qui se met à dormir alors il y a interblocage.
  cree_processus(bash, "bash");

  enable_timer();
  idle();

  /* on ne doit jamais sortir de kernel_start */
  while (1) {
    /* cette fonction arrete le processeur */
    hlt();
  }
}
