// TODO: afficher un prompt "$ " avant chaque commande
// ajouter une commande exit pour quitter le shell (donc break de la boucle for
// en cours)
// ajouter un commande bash pour lancer un nouveau shell

#include "shell.h"
#include "cpu.h"
#include "process.h"
#include "string.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>

extern int ps();
extern int help();
extern int f_true();
extern int f_false();
extern int test_wait();
extern int history();
volatile int last_index_cmd = 0;

char c[MAX_LENGTH_COMMANDS];
volatile char commands[MAX_LENGTH_COMMANDS];

// Renvoie la commande tapée par l'utilisateur (une commande termine par un
// '\n') en remplaçant le '\n' à la fin par un '\0' pour les strcmp ensuite,
// puis vide le buffer commands.
static char *get_command() {
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
      cree_processus(target_function, target_command);
      return 1;
    }
  }

  return 0;
}

// Invite de commande.
// TODO: créer une structure command_t qui contient le nom de la commande et
// l'adresse de la fonction à exécuter. Il suffit alors de remplacer le big if
// par une boucle for sur le tableau contenant toutes les commande_t.
int bash() {
  for (;;) {
    char *command = get_command();
    if (exec_command(command, "help", help)) {
    } else if (exec_command(command, "ps", ps)) {
    } else if (exec_command(command, "history", history)) {
    } else if (exec_command(command, "true", f_true)) {
    } else if (exec_command(command, "false", f_false)) {
    } else if (exec_command(command, "test_wait", test_wait)) {
    } else if (strcmp(command, "\0") == 0) {
      // On ne fait rien, l'utilisateur a simplement appuyé sur entrée.
    } else {
      printf("Commande introuvable.\n");
    }
  }

  return 0;
}
