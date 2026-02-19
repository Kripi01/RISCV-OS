// TODO: Dans tous les fichiers, utiliser au maximum des static pour
// l'encapsulation

#include "buddy_heap.h"
#include "cpu.h"
#include "interrupt.h"
#include "keyboard.h"
#include "platform.h"
#include "pm.h"
#include "process.h"
#include "screen.h"
#include "shell.h"
#include "syscalls.h"
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

// idle est le seul processus à être exécuté en mode S (pour le wfi)
void idle() {
  _place_curseur(1, 0); // On affiche le premier curseur

  for (;;) {
    s_enable_it();
    hlt();
    s_disable_it();
  }
}

// BUG: le code des fonctions apelées dans un processus doit se situer après le
// code du processus car dans load_process on copie le code du processus à
// partir de 0x40000000, donc les offsets négatifs résultent en des adresses
// virtuelles du genre 0x3FFFFFE0, ce qui provoque un segfault (pourtant ça
// marche...)
int user_process_test3() {
  while (1) {
    UPUTC('3');
  }
  return 0;
}

int user_process_test2() {
  UCREE_PROCESSUS(user_process_test3, "test3");
  while (1) {
    UPUTC('2');
  }
  return 0;
}

int user_process_test() {
  uint64_t pid = UCREE_PROCESSUS(user_process_test2, "test2");
  UPRINTF("%d\n", pid, 0, 0, 0, 0, 0);
  while (1) {
    UPUTC('1');
  }
  UEXIT(0);
}

void kernel_start() {
  init_uart();
  enable_uart_interrupts();
  init_ecran();

  buddy_init_heap();

  init_traitant(mon_traitant); // pour le mode S
  enable_timer();

  init_frames();

  init_proc(); // crée idle et l'élit.

  // WARNING: il ne faut pas faire créer le premier bash par idle (avec waitpid)
  // car sinon, idle serait en attente de la mort de bash et quand bash lance un
  // processus qui se met à dormir alors il y a interblocage.
  cree_processus(bash, "bash");
  // cree_processus(user_process_test, "user_test");

  idle();

  /* on ne doit jamais sortir de kernel_start */
  while (1) {
    /* cette fonction arrete le processeur */
    hlt();
  }
}
