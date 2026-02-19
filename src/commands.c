#include "commands.h"
#include "syscalls.h"
#include <stddef.h>
#include <stdint.h>

// // BUG:
// int proc1() {
//   for (int32_t i = 0; i < 2; i++) {
//     printf("[temps = %u] processus %s pid = %li\n", nbr_secondes(),
//     mon_nom(),
//            mon_pid());
//     dors(2);
//   }
//
//   return 0;
// }
//
// // BUG:
// int proc2() {
//   for (int32_t i = 0; i < 2; i++) {
//     printf("[temps = %u] processus %s pid = %li\n", nbr_secondes(),
//     mon_nom(),
//            mon_pid());
//     dors(3);
//   }
//   cree_processus(proc1, "new_proc1");
//
//   return 0;
// }
//
// // BUG:
// int proc3() {
//   for (int32_t i = 0; i < 2; i++) {
//     printf("[temps = %u] processus %s pid = %li\n", nbr_secondes(),
//     mon_nom(),
//            mon_pid());
//     dors(5);
//   }
//   cree_processus(proc2, "new_proc2");
//
//   return 0;
// }
//

int segfault_test() {
  int *p = NULL;
  *p = 42;
  UPRINTF("p: %p, *p: %d\n", p, *p, 0, 0, 0, 0);
  UEXIT(0);
}

int help() {
  UPUTS("Commandes disponibles: help, ps, true, false, bash, clear, "
        "segfault_test\n");
  UEXIT(0);
}

int f_true() { UEXIT(0); }

int f_false() { UEXIT(1); }

int history() {
  UPUTS("todo\n");
  UEXIT(0);
}

int clear() {
  UPUTS("\f");
  UEXIT(0);
}

int fg() {
  UPUTS("todo\n");
  UEXIT(0);
}

// ps accède aux données des processus qui sont stockées une seule fois en
// mémoire kernel donc on est obligé de faire un syscall.
int ups() {
  int exit_code = UPS();
  UEXIT(exit_code);
}
