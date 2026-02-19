#ifndef __SYSCALLS_H__
#define __SYSCALLS_H__

#include "platform.h"
#include <stdint.h>

// WARNING: Les fonctions syscalls DOIVENT être inlinées (__inline__ ne marche
// pas car on compile avec -O0) car sinon elles seraient stockées en mémoire
// kernel et les processus ne pourraient pas y accèder.

// Fait un syscall pour le code de fonction fourni (=a7) dont les arguments sont
// de a0 à a6 (probablement pas tous utilisés e.g. a1 pour uputc -> dans ce cas,
// la valeur de a1 est arbitraire, par convention on prend 0). On retourne la
// valeur de retour de la fonction kernel (qui est la valeur de retour de
// trap_handler).
// NOTE: on utilise une extension de GCC pour que le bloc
// s'évalue à sa dernière expression [ pour pouvoir faire uint64_t a0 =
// SYSCALL(code, a0, a1)
// ]
#define SYSCALL(code, a0, a1)                                                  \
  ({                                                                           \
    __asm__("mv a7, %0" ::"r"((code)));                                        \
    __asm__("mv a0, %0" ::"r"((a0)));                                          \
    __asm__("mv a1, %0" ::"r"((a1)));                                          \
    __asm__("ecall");                                                          \
    uint64_t res;                                                              \
    __asm__("mv %0, a0" : "=r"((res)));                                        \
    res;                                                                       \
  })

#define UPUTC(c) SYSCALL(CODE_UPUTC, (uint64_t)(c), 0)

#define UPUTS(str) SYSCALL(CODE_UPUTS, (uint64_t)(str), 0)

#define UCREE_PROCESSUS(proc_code, nom)                                        \
  SYSCALL(CODE_UCREE_PROCESSUS, (uint64_t)(proc_code), (uint64_t)nom)

#endif // __SYSCALLS_H__
