#include "commands.h"
#include "syscalls.h"
#include <stddef.h>
#include <stdint.h>

int proc1() {
  for (int32_t i = 0; i < 2; i++) {
    uint64_t nb = UNBR_SECONDES();
    uint64_t nom = UMON_NOM();
    uint64_t pid = UMON_PID();
    UPRINTF("[temps = %u] processus %s pid = %li\n", nb, nom, pid, 0, 0, 0);
    UDORS(2);
  }

  UEXIT(0);
}

int proc2() {
  UCREE_PROCESSUS(proc1, "new_proc1");
  for (int32_t i = 0; i < 2; i++) {
    uint64_t nb = UNBR_SECONDES();
    uint64_t nom = UMON_NOM();
    uint64_t pid = UMON_PID();
    UPRINTF("[temps = %u] processus %s pid = %li\n", nb, nom, pid, 0, 0, 0);
    UDORS(3);
  }

  UEXIT(0);
}

int proc3() {
  UCREE_PROCESSUS(proc2, "new_proc2");
  for (int32_t i = 0; i < 2; i++) {
    uint64_t nb = UNBR_SECONDES();
    uint64_t nom = UMON_NOM();
    uint64_t pid = UMON_PID();
    UPRINTF("[temps = %u] processus %s pid = %li\n", nb, nom, pid, 0, 0, 0);
    UDORS(5);
  }

  UEXIT(0);
}

int segfault_test() {
  int *p = NULL;
  *p = 42;
  UPRINTF("p: %p, *p: %d\n", p, *p, 0, 0, 0, 0);
  UEXIT(0);
}

int help() {
  UPUTS("Commandes disponibles: help, ps, true, false, bash, clear, "
        "segfault_test, proc1, proc2, proc3, france\n");
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

// ps accède aux données des processus (scheduler) qui sont stockées une seule
// fois en mémoire kernel donc on est obligé de faire un syscall.
int ups() {
  int exit_code = UPS();
  UEXIT(exit_code);
}

int france() {
  UFRANCE();
  UEXIT(0);
}
