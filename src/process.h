#ifndef __PROCESS_H__
#define __PROCESS_H__

#include <stdint.h>

#define MAX_CHAR_NAME 100
#define PROCESS_STACK_SIZE 4096
#define MAX_NB_PROCESSES 100

typedef enum {
  ELU,
  ACTIVABLE,
  ENDORMI,
} Status;

typedef struct {
  int pid;
  char nom[MAX_CHAR_NAME];
  Status etat;
  uint64_t contexte[18];
  uint64_t pile[PROCESS_STACK_SIZE];
  uint64_t heure_reveil;
} process_t;

void init_proc();
int64_t cree_processus(void code(), char *nom);
void ordonnance();
int64_t mon_pid();
char *mon_nom();
void dors(uint64_t nbr_secs);

extern void idle();
extern void ctx_sw(uint64_t *contexte1, uint64_t *contexte2);

#endif
