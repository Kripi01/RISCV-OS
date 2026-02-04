#ifndef __SHELL_H__
#define __SHELL_H__

#define MAX_LENGTH_COMMANDS 1000

extern volatile int last_index_cmd;
extern volatile char command_str[MAX_LENGTH_COMMANDS];

typedef struct {
  char *nom;
  int (*fonction)();
} command_t;

int bash();

#endif // __SHELL_H__
