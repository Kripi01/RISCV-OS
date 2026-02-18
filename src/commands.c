#include "commands.h"
#include "interrupt.h"
#include "process.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

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

int segfault_test() {
  int *p = NULL;
  *p = 42;
  printf("p: %p, *p: %d\n", p, *p);
  return 0;
}

int double_mapping_test() {
  uint8_t variable = 42;
  uint8_t *ptr1 = &variable;
  uint8_t *ptr2 = ptr1 + 0x80000000; /* + 2 GiB */

  printf("ptr1 @%p = %u\n", ptr1, *ptr1);
  printf("ptr2 @%p = %u\n", ptr2, *ptr2);
  if (*ptr1 == *ptr2)
    printf("Test mémoire virtuelle OK\n");
  else
    printf("Test mémoire virtuelle FAIL\n");

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
