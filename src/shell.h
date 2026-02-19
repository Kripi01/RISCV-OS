#ifndef __SHELL_H__
#define __SHELL_H__

#define MAX_LENGTH_COMMANDS 1000

extern volatile int last_index_cmd;
extern volatile char command_str[MAX_LENGTH_COMMANDS];

typedef struct {
  char *nom;
  int (*fonction)();
} command_t;

char *get_command(char va[MAX_LENGTH_COMMANDS]);
int exec_command(char *cmd_str, char *target_cmd_str, int target_fct());
int bash();

#endif // __SHELL_H__
