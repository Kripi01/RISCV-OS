// Macros variadiques: https://gcc.gnu.org/onlinedocs/cpp/Variadic-Macros.html
// Statements and Declarations in Expressions (GCC):
// https://gcc.gnu.org/onlinedocs/gcc/Statement-Exprs.html

#ifndef __SYSCALLS_H__
#define __SYSCALLS_H__

// Les syscalls sont des macros pour forcer l'inlining (compilation avec -O0)
// pour que les fonctions soient dans le même code que les processus (!= kernel)

#include "platform.h"
#include <stdarg.h>
#include <stdint.h>

// Fait un appel système: code de fonction dans a7, arguments dans a0-a6 (pas
// tous utilisés => convention = 0).
// On utilise l'extension GCC ({...}) pour retourner la valeur de a0 après le
// ecall [pour pouvoir faire ensuite: uint64_t a0 = SYSCALL(code, a0, a1);]
#define SYSCALL(code, a0, a1, a2, a3, a4, a5, a6)                              \
  ({                                                                           \
    __asm__("mv a7, %0" ::"r"((code)));                                        \
    __asm__("mv a0, %0" ::"r"((a0)));                                          \
    __asm__("mv a1, %0" ::"r"((a1)));                                          \
    __asm__("mv a2, %0" ::"r"((a2)));                                          \
    __asm__("mv a3, %0" ::"r"((a3)));                                          \
    __asm__("mv a4, %0" ::"r"((a4)));                                          \
    __asm__("mv a5, %0" ::"r"((a5)));                                          \
    __asm__("mv a6, %0" ::"r"((a6)));                                          \
    __asm__("ecall");                                                          \
    uint64_t _res;                                                             \
    __asm__("mv %0, a0" : "=r"((_res)));                                       \
    _res;                                                                      \
  })

// Syscalls d'affichage écran/terminal
#define UPUTC(c) SYSCALL(CODE_UPUTC, (uint64_t)(c), 0, 0, 0, 0, 0, 0)
#define UPUTS(str) SYSCALL(CODE_UPUTS, (uint64_t)(str), 0, 0, 0, 0, 0, 0)
// WARNING: La liste de variables à printf ne doit pas dépasser 6 !!!!
#define UPRINTF(fmt, ...) SYSCALL(CODE_UPRINTF, fmt, ##__VA_ARGS__)
#define UFRANCE() SYSCALL(CODE_UFRANCE, 0, 0, 0, 0, 0, 0, 0)

// Syscalls processus
#define UCREE_PROCESSUS(proc_code, nom)                                        \
  (int64_t)SYSCALL(CODE_UCREE_PROCESSUS, (uint64_t)(proc_code),                \
                   (uint64_t)(nom), 0, 0, 0, 0, 0)

#define UWAITPID(pid) SYSCALL(CODE_UWAITPID, (uint64_t)(pid), 0, 0, 0, 0, 0, 0)

#define UEXIT(exit_code)                                                       \
  return SYSCALL(CODE_UEXIT, (uint64_t)(exit_code), 0, 0, 0, 0, 0, 0)

#define UPS() SYSCALL(CODE_UPS, 0, 0, 0, 0, 0, 0, 0)
#define UDORS(t) SYSCALL(CODE_UDORS, (uint64_t)(t), 0, 0, 0, 0, 0, 0)
#define UNBR_SECONDES() SYSCALL(CODE_UNBR_SECONDES, 0, 0, 0, 0, 0, 0, 0)
#define UMON_NOM() SYSCALL(CODE_UMON_NOM, 0, 0, 0, 0, 0, 0, 0)
#define UMON_PID() SYSCALL(CODE_UMON_PID, 0, 0, 0, 0, 0, 0, 0)

// Syscalls bash
#define UGET_COMMAND(va)                                                       \
  (char *)SYSCALL(CODE_UGET_COMMAND, (uint64_t)(va), 0, 0, 0, 0, 0, 0)

#define UEXEC_COMMAND(cmd, target_cmd, target_fct)                             \
  (int)SYSCALL(CODE_UEXEC_COMMAND, (uint64_t)(cmd), (uint64_t)(target_cmd),    \
               (uint64_t)(target_fct), 0, 0, 0, 0)

#endif // __SYSCALLS_H__
