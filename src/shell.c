#include "shell.h"
#include "cpu.h"
#include "process.h"
#include "string.h"
#include "tests.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>

extern int ps();
extern int help();
extern int f_true();
extern int f_false();
extern int proc1();
extern int proc2();
extern int proc3();
extern int history();
extern int fg();
extern int clear();
extern int buddy_heap_test();
extern int buddy_heap_overflow_test();
extern int segfault_test();
extern int double_mapping_test();

command_t commands[] = {
    {.nom = "help", .fonction = help},
    {.nom = "ps", .fonction = ps},
    {.nom = "history", .fonction = history},
    {.nom = "true", .fonction = f_true},
    {.nom = "false", .fonction = f_false},
    {.nom = "proc1", .fonction = proc1},
    {.nom = "proc2", .fonction = proc2},
    {.nom = "proc3", .fonction = proc3},
    {.nom = "bash", .fonction = bash},
    {.nom = "fg", .fonction = fg},
    {.nom = "clear", .fonction = clear},
    {.nom = "buddy_heap_test", .fonction = buddy_heap_test},
    {.nom = "buddy_heap_overflow_test", .fonction = buddy_heap_overflow_test},
    {.nom = "segfault_test", .fonction = segfault_test},
    {.nom = "double_mapping_test", .fonction = double_mapping_test},
};

#define NB_COMMANDS (int)(sizeof(commands) / sizeof(command_t))

volatile int last_index_cmd = 0;

char c[MAX_LENGTH_COMMANDS];
volatile char command_str[MAX_LENGTH_COMMANDS];

// Renvoie la commande tapée par l'utilisateur (une commande termine par un
// '\n') en remplaçant le '\n' à la fin par un '\0' pour les strcmp ensuite,
// puis vide le buffer commands.
static char *get_command() {
  while (command_str[last_index_cmd] != '\n') {
    s_enable_it();
    hlt();
    s_disable_it();
  }

  // On remplace le '\n' à  la fin par un '\0'
  strncpy(c, (char *)command_str, last_index_cmd);
  c[last_index_cmd] = '\0';

  // Puis on vide la liste des commandes.
  command_str[last_index_cmd] = '\0'; // Il faut enlever le '\n' à la fin
  last_index_cmd = 0;

  return c;
}

// Éxecute la commande cible (avec filiation ou non).
// Renvoie 1 si la commande tapée par l'utilisateur correspond à la commande
// cible (éventuellement avec ' &' à la fin) et 0 sinon.
// TODO: gérer le cas où pid == -1
static int exec_command(char *command, char *target_command,
                        int target_function()) {
  size_t len_target = strlen(target_command);

  // On vérifie si la commande commence par le bon nom.
  if (strncmp(command, target_command, len_target) == 0) {
    // Si la commande est en avant-plan (pas de ' &')
    if (command[len_target] == '\0') {
      int pid = cree_processus(target_function, target_command);
      waitpid(pid); // On attend que le processus fils soit mort.
      return 1;
    }

    // Si la commande est en arrière-plan (il y a un ' &')
    if (strcmp(command + len_target, " &") == 0) {
      // TODO: faire des jobs (fonctionnalité du shell != kernel)
      // et gérer le cas particulier de bash en arrière-plan.
      cree_processus(target_function, target_command);
      return 1;
    }
  }

  return 0;
}

// Invite de commande.
int bash() {
  for (;;) {
    // NOTE: Fonctionne en filiation, mais pas en fond de tâche (mais c'est
    // normal).
    // BUG: Quand on spamme entrée, le "$ " s'affiche en saccadé, parce que bash
    // n'a pas la main très souvent, il partage trop avec idle. Il faut changer
    // la politique d'ordonnancement, pour donner plus de temps à bash et moins
    // à idle ??
    printf("$ ");
    char *cur_command = get_command();

    // Si l'utilisateur tape simplement entrée, on ne l'interprète pas
    if (strcmp(cur_command, "\0") == 0) {
      continue;
    }

    if (strcmp(cur_command, "exit") == 0) {
      break;
    }

    int found = 0;
    for (int i = 0; i < NB_COMMANDS; i++) {
      command_t *command = &commands[i];
      if (exec_command(cur_command, command->nom, command->fonction)) {
        found = 1;
        break;
      }
    }

    if (!found) {
      printf("Commande introuvable.\n");
    }
  }

  return 0;
}
