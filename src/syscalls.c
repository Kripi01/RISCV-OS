#include "syscalls.h"
#include <stdint.h>

static void syscall(uint64_t code, uint64_t a6) {
  // WARNING: On ne peut pas utiliser a0, a1 et a2 car ils sont utilisés comme
  // arguments du s_trap_handler
  __asm__("mv a7, %0" ::"r"(code));
  __asm__("mv a6, %0" ::"r"(a6));
  __asm__("ecall");
}

void uputc(char c) {
  // BUG: si on fait un uputs avec un char * en arugment alors l'adresse est
  // virtuelle, donc segfault pour le mode S
  syscall(0, (uint64_t)c);
}
