#include "commands.h"
#include "syscalls.h"
#include <stddef.h>
#include <stdint.h>

int proc1(int argc, char **argv) {
  for (int32_t i = 0; i < 2; i++) {
    uint64_t nb = UNBR_SECONDES();
    uint64_t nom = UMON_NOM();
    uint64_t pid = UMON_PID();
    UPRINTF("[temps = %u] processus %s pid = %li\n", nb, nom, pid, 0, 0, 0);
    UDORS(2);
  }

  UEXIT(0);
}

int proc2(int argc, char **argv) {
  UCREE_PROCESSUS(proc1, argc, argv);
  for (int32_t i = 0; i < 2; i++) {
    uint64_t nb = UNBR_SECONDES();
    uint64_t nom = UMON_NOM();
    uint64_t pid = UMON_PID();
    UPRINTF("[temps = %u] processus %s pid = %li\n", nb, nom, pid, 0, 0, 0);
    UDORS(3);
  }

  UEXIT(0);
}

// BUG:
int proc3(int argc, char **argv) {
  UCREE_PROCESSUS(proc2, argc, argv);
  for (int32_t i = 0; i < 2; i++) {
    uint64_t nb = UNBR_SECONDES();
    uint64_t nom = UMON_NOM();
    uint64_t pid = UMON_PID();
    UPRINTF("[temps = %u] processus %s pid = %li\n", nb, nom, pid, 0, 0, 0);
    UDORS(5);
  }

  UEXIT(0);
}

int segfault_test(int argc, char **argv) {
  int *p = NULL;
  *p = 42;
  UPRINTF("p: %p, *p: %d\n", p, *p, 0, 0, 0, 0);
  UEXIT(0);
}

int help(int argc, char **argv) {
  UPUTS("Commandes disponibles: help, ps, true, false, bash, clear, "
        "segfault_test, proc1, proc2, proc3, france\n");
  UEXIT(0);
}

int f_true(int argc, char **argv) { UEXIT(0); }

int f_false(int argc, char **argv) { UEXIT(1); }

int history(int argc, char **argv) {
  UPUTS("todo\n");
  UEXIT(0);
}

int clear(int argc, char **argv) {
  UPUTS("\f");
  UEXIT(0);
}

int fg(int argc, char **argv) {
  UPUTS("todo\n");
  UEXIT(0);
}

// ps accède aux données des processus (scheduler) qui sont stockées une seule
// fois en mémoire kernel donc on est obligé de faire un syscall.
int ups(int argc, char **argv) {
  int exit_code = UPS();
  UEXIT(exit_code);
}

// Affiche le drapeau de la France en arrière-plan.
int france(int argc, char **argv) {
  UFRANCE();
  UEXIT(0);
}

int echo(int argc, char *argv[]) {
  for (int i = 1; i < argc; i++) {
    UPRINTF("%s ", argv[i], 0, 0, 0, 0, 0);
  }
  UPUTC('\n');

  UEXIT(0);
}
