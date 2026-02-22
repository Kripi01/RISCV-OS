#ifndef __SHELL_H__
#define __SHELL_H__

#define MAX_LENGTH_COMMANDS 1000
#define MAX_NB_ARGS 100

extern volatile int last_index_cmd;
extern volatile char command_str[MAX_LENGTH_COMMANDS];

typedef struct {
  char *nom;
  int (*fonction)();
} command_t;

char *get_raw_argv(char *raw_argv);
int exec_command(char **argv, char *target_cmd, int argc,
                 int target_fct(int, char **));
int bash(int argc, char **argv);

#endif // __SHELL_H__
