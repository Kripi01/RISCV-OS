#include "syscalls.h"
#include "platform.h"
#include <stdint.h>

// Fait un syscall pour le code de fonction fourni (=a7) dont les arguments sont
// de a0 à a6 (probablement pas tous utilisés e.g. a1 pour uputc -> dans ce cas,
// la valeur de a1 est arbitraire, par convention on prend 0)
static uint64_t syscall(uint64_t code, uint64_t a0, uint64_t a1) {
  __asm__("mv a7, %0" ::"r"(code));
  __asm__("mv a0, %0" ::"r"(a0));
  __asm__("mv a1, %0" ::"r"(a1));
  __asm__("ecall");

  // on retourne la valeur de retour de la fonction kernel = la valeur de retour
  // de trap_handler = a0
  uint64_t res;
  __asm__("mv %0, a0" : "=r"(res));
  return res;
}

void uputc(char c) { syscall(UPUTC, (uint64_t)c, 0); }

void uputs(char *str) { syscall(UPUTS, (uint64_t)str, 0); }

uint64_t ucree_processus(int proc_code(), char *nom) {
  return syscall(UCREE_PROCESSUS, (uint64_t)proc_code, (uint64_t)nom);
}
