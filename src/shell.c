#include "shell.h"
#include "cpu.h"
#include "process.h"
#include "string.h"
#include "syscalls.h"
#include "uheap.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

// TODO: faire des variables d'environnement ($?, PWD, etc)
// TODO: mettre le dossier courant dans le prompt

extern int ups(int argc, char **argv);
extern int help(int argc, char **argv);
extern int f_true(int argc, char **argv);
extern int f_false(int argc, char **argv);
extern int proc1(int argc, char **argv);
extern int proc2(int argc, char **argv);
extern int proc3(int argc, char **argv);
extern int history(int argc, char **argv);
extern int fg(int argc, char **argv);
extern int clear(int argc, char **argv);
extern int uheap_test(int argc, char **argv);
extern int uheap_overflow_test(int argc, char **argv);
extern int segfault_test(int argc, char **argv);
extern int france(int argc, char **argv);
extern int echo(int argc, char **argv);
extern int umkdir(int argc, char **argv);
extern int uls(int argc, char **argv);
extern int upwd(int argc, char **argv);

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
    {.nom = "uheap_test", .fonction = uheap_test},
    {.nom = "uheap_overflow_test", .fonction = uheap_overflow_test},
    {.nom = "segfault_test", .fonction = segfault_test},
    {.nom = "france", .fonction = france},
    {.nom = "echo", .fonction = echo},
    {.nom = "mkdir", .fonction = umkdir},
    {.nom = "ls", .fonction = uls},
    {.nom = "pwd", .fonction = upwd},
};

#define NB_COMMANDS (int)(sizeof(commands) / sizeof(command_t))

volatile int last_index_cmd = 0;

volatile char command_str[MAX_LENGTH_COMMANDS];

// Retourne la commande tapée par l'utilisateur si disponible (une commmande
// doit terminer par un '\n') et NULL sinon. Remplace le '\n' à la fin par un
// '\0' pour les strcmp ensuite.
//  WARNING: Doit être exécutée en mode S (car command_str est une adresse
//  kernel, modifiée dans keyboard.c).
char *get_raw_argv(char *raw_argv) {
  // Pas d'overflow car last_index_cmd < MAX_LENGTH_COMMANDS (keyboard.c)
  if (command_str[last_index_cmd] != '\n') {
    return NULL;
  }

  // On remplace le '\n' à  la fin par un '\0'
  enable_sum(); // on accède à une adresse virtuelle de bash -> bit SUM
  strncpy(raw_argv, (char *)command_str, last_index_cmd);
  raw_argv[last_index_cmd] = '\0';
  disable_sum();

  // Puis on vide la liste des commandes.
  // On enlève le '\n' à la fin (cf. keyboard.c)
  command_str[last_index_cmd] = '\0';
  last_index_cmd = 0;

  return raw_argv;
}

// Retourne la commande parsée (comme le argv de la fonction main en C) allouée
// sur le tas. Écrit également dans argc le nb d'arguments de la commande (nom
// de la commande inclu).
// WARNING: Doit être exécutée en mode U (car on utilise le tas de bash).
static char **parse_arguments(char *raw_argv, int *argc) {
  char **argv = umalloc(MAX_NB_ARGS * sizeof(char *));
  if (argv == NULL) {
    UPUTS("bash error: pas assez de place sur le tas.\n");
    return NULL;
  }

  int i = 0; // la taille de l'argument courant

  int local_argc = 0;
  int raw_i = 0;
  char c = raw_argv[0];
  // TODO: gérer le cas des deux (ou plus) espaces consécutifs
  while (1) {
    if (c == ' ' || c == '\0') {
      raw_argv[raw_i] = '\0';
      if ((argv[local_argc] = umalloc(i * sizeof(char))) == NULL) {
        UPUTS("bash error: pas assez de place sur le tas.\n");
        return NULL;
      };

      strcpy(argv[local_argc], raw_argv + raw_i - i); // ça copie aussi le '\0'
      local_argc++;
      i = 0;

      if (c == '\0') {
        break;
      }
    } else {
      i++;
    }

    c = raw_argv[++raw_i];
  }

  *argc = local_argc;
  return argv;
}

// Éxecute la commande cible (avec filiation ou non) avec les arguments fournis.
// Renvoie 1 si la commande tapée par l'utilisateur correspond à la commande
// cible (éventuellement avec ' &' à la fin) et 0 sinon.
// WARNING: Cette fonction doit être exécutée en mode S
int exec_command(char **argv, char *target_cmd, int argc,
                 int target_fct(int, char **)) {
  size_t len_target = strlen(target_cmd);
  char *base_cmd = argv[0];

  if (strncmp(base_cmd, target_cmd, len_target) == 0) {
    // Si la commande est en arrière-plan (il y a un ' &' en denier argument)
    if (strcmp(argv[argc - 1], "&") == 0) {
      // TODO: faire des jobs et gérer le cas particulier de bash en bg
      cree_processus(target_fct, argc, argv);
      return 1;
    } else { // la commande est en avant-plan
      int pid = cree_processus(target_fct, argc, argv);
      if (pid <= 0) {
        return 0;
      }
      waitpid(pid); // On attend que le processus fils soit mort.
      return 1;
    }
  }

  return 0;
}

// Invite de commande.
int bash(int bash_argc, char **bash_argv) {
  // On alloue le buffer des arguments bruts sur la pile de bash
  char raw_argv[MAX_LENGTH_COMMANDS];
  int argc = 0;

  for (;;) {
    // NOTE: Fonctionne en filiation, mais pas en bg (mais c'est normal).
    // BUG: Quand on spamme entrée, le "$ " s'affiche en saccadé, parce que bash
    // n'a pas la main très souvent, il partage trop avec idle. Il faut changer
    // la politique d'ordonnancement, pour donner plus de temps à bash et moins
    // à idle ??
    UPUTS("$ ");
    while (UGET_RAW_ARGV(raw_argv) == NULL) {
      // TODO: ajouter un état EN_ATTENTE_IO pour éviter l'attente active (on
      // endort(-io) bash jusqu'à ce que get_command ne renvoie pas NULL -> on
      // réveille bash dans keyboard.c, à l'interruption clavier)
      __asm__ volatile("nop"); // attente active (pas ouf)
    }

    // argv est alloué sur le tas
    char **argv = parse_arguments(raw_argv, &argc);

    // Si l'utilisateur tape juste entrée, on ne l'interprète pas
    if (argv[0][0] == '\0') {
      continue;
    }

    if (strcmp(argv[0], "exit") == 0) {
      break;
    }

    int found = 0;
    for (int i = 0; i < NB_COMMANDS; i++) {
      command_t *command = &commands[i];
      // TODO: récupérer le pid du processus créé pour l'afficher avec echo $?
      if (UEXEC_COMMAND(argv, command->nom, argc, command->fonction)) {
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
