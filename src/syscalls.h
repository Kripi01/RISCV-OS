#ifndef __SYSCALLS_H__
#define __SYSCALLS_H__

#include <stdint.h>

void uputc(char c);
void uputs(char *str);
uint64_t ucree_processus(int proc_code(), char *nom);

#endif // __SYSCALLS_H__
