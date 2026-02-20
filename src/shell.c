#include "shell.h"
#include "cpu.h"
#include "process.h"
#include "string.h"
#include "syscalls.h"

extern int ups();
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
extern int france();

command_t commands[] = {
    {.nom = "help", .fonction = help},
    {.nom = "ps", .fonction = ups},
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
    {.nom = "france", .fonction = france},
};

#define NB_COMMANDS (int)(sizeof(commands) / sizeof(command_t))

volatile int last_index_cmd = 0;

volatile char command_str[MAX_LENGTH_COMMANDS];

// Renvoie la commande tapée par l'utilisateur si disponible (une commande
// termine par un '\n') et NULL sinon.
// Remplace le '\n' à la fin par un '\0' pour les strcmp ensuite.
// Vide également le cmd_str_buf qui est l'adresse virtuelle (dans la zone user
// de bash) du buffer pour les noms des commandes tapées.
// WARNING: Cette fonction doit être exécutée en mode S (car command_str est une
// adresse kernel car modifié dans keyboard.c)
char *get_command(char cmd_str_buf[MAX_LENGTH_COMMANDS]) {
  // Pas d'overflow car last_index_cmd < MAX_LENGTH_COMMANDS (keyboard.c)
  if (command_str[last_index_cmd] != '\n') {
    return NULL;
  }

  // On remplace le '\n' à  la fin par un '\0'
  enable_sum(); // on accède à une adresse virtuelle de bash -> bit SUM
  strncpy(cmd_str_buf, (char *)command_str, last_index_cmd);
  cmd_str_buf[last_index_cmd] = '\0';
  disable_sum();

  // Puis on vide la liste des commandes.
  command_str[last_index_cmd] = '\0'; // Il faut enlever le '\n' à la fin
  last_index_cmd = 0;

  return (char *)cmd_str_buf;
}

// Éxecute la commande cible (avec filiation ou non).
// Renvoie 1 si la commande tapée par l'utilisateur correspond à la commande
// cible (éventuellement avec ' &' à la fin) et 0 sinon.
// WARNING: Cette fonction doit être exécutée en mode S
int exec_command(char *cmd_str, char *target_cmd_str, int target_fct()) {
  size_t len_target = strlen(target_cmd_str);

  // On vérifie si la commande commence par le bon nom.
  if (strncmp(cmd_str, target_cmd_str, len_target) == 0) {
    // Si la commande est en avant-plan (pas de ' &')
    if (cmd_str[len_target] == '\0') {
      int pid = cree_processus(target_fct, target_cmd_str);
      if (pid <= 0) {
        return 0;
      }
      waitpid(pid); // On attend que le processus fils soit mort.
      return 1;
    }

    // Si la commande est en arrière-plan (il y a un ' &')
    if (strcmp(cmd_str + len_target, " &") == 0) {
      // TODO: faire des jobs (fonctionnalité du shell != kernel)
      // et gérer le cas particulier de bash en arrière-plan.
      cree_processus(target_fct, target_cmd_str);
      return 1;
    }
  }

  return 0;
}

// Invite de commande.
int bash() {
  // on alloue le buffer des commandes sur la pile de bash -> adresse virtuelle
  char cmd_str_buf[MAX_LENGTH_COMMANDS];

  for (;;) {
    // NOTE: Fonctionne en filiation, mais pas en fond de tâche (mais c'est
    // normal).
    // BUG: Quand on spamme entrée, le "$ " s'affiche en saccadé, parce que bash
    // n'a pas la main très souvent, il partage trop avec idle. Il faut changer
    // la politique d'ordonnancement, pour donner plus de temps à bash et moins
    // à idle ??
    UPUTS("$ ");
    char *cur_command = NULL;
    while ((cur_command = UGET_COMMAND(cmd_str_buf)) == NULL) {
      // TODO: ajouter un état EN_ATTENTE_IO pour éviter l'attente active (on
      // endort(-io) bash jusqu'à ce que get_command ne renvoie pas NULL -> on
      // réveille bash dans keyboard.c, à l'interruption clavier)
      __asm__ volatile("nop"); // attente active (pas ouf)
    }

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
      if (UEXEC_COMMAND(cur_command, command->nom, command->fonction)) {
        found = 1;
        break;
      }
    }

    if (!found) {
      UPUTS("Commande introuvable.\n");
    }
  }

  UEXIT(0);
}
