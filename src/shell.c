#include "shell.h"
#include "cpu.h"
#include "process.h"
#include "string.h"
#include <stdio.h>
#include <string.h>

extern int ps();
extern int help();
extern int f_true();
extern int f_false();
volatile int last_index_cmd = 0;

char c[MAX_LENGTH_COMMANDS];
volatile char commands[MAX_LENGTH_COMMANDS];

// TODO: ajouter un \0 à la fin de la commande pour le strcmp ensuite.
char *get_command() {
  while (commands[last_index_cmd] != '\n') {
    enable_it();
    hlt();
    disable_it();
  }

  // On remplace le '\n' à  la fin par un '\0'
  strncpy(c, (char *)commands, last_index_cmd);
  c[last_index_cmd] = '\0';

  // Puis on vide la liste des commandes.
  memset((char *)commands, 0, MAX_LENGTH_COMMANDS);
  last_index_cmd = 0;

  return c;
}

// TODO: Implémenter une hash table à la place du big if
int bash() {
  for (;;) {
    char *s = get_command();
    if (strcmp(s, "help") == 0) {
      cree_processus(help, "help");
    } else if (strcmp(s, "ps") == 0) {
      cree_processus(ps, "ps");
    } else if (strcmp(s, "history") == 0) {
      //
    } else if (strcmp(s, "true") == 0) {
      cree_processus(f_true, "true");
    } else if (strcmp(s, "false") == 0) {
      cree_processus(f_false, "false");
    } else if (strcmp(s, "\0") == 0) {
      // On ne fait rien, l'utilisateur a simplement appuyé sur entrée.
    } else {
      printf("Commande introuvable.\n");
    }
  }

  return 0;
}
