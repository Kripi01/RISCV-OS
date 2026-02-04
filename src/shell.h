#ifndef __SHELL_H__
#define __SHELL_H__

#define MAX_LENGTH_COMMANDS 1000
extern volatile int last_index_cmd;
extern volatile char commands[MAX_LENGTH_COMMANDS];

char *get_command();
int bash();

#endif // __SHELL_H__
