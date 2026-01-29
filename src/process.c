#include "process.h"
#include <stdint.h>
#include <string.h>

static int next_pid = 0;
static process_t scheduler[MAX_NB_PROCESSES];
static process_t *actif;

// Initialise (ou de réinitialise) le système pour la gestion des processus.
// Cette fonction ne démarre pas de nouveau processus.
void init_proc() {
  // Le processus de pid 0 est toujours nommé idle. Par défaut, c'est lui qui
  // est actif car dans un système, il doit toujours y avoir au moins un
  // processus actif, car le système ne doit jamais s'arrêter.
  process_t *p = &scheduler[0];
  p->pid = 0;
  strncpy(p->nom, "idle", MAX_CHAR_NAME);
  p->etat = ELU;
  p->contexte[0] = (uint64_t)idle;
  p->contexte[1] = (uint64_t)(p->pile + PROCESS_STACK_SIZE);

  actif = p;
  next_pid = 1;
}

// Renvoie le pid du processus créé, ou -1 en cas d'erreur
int64_t cree_processus(void code(), char *nom) {
  if (next_pid >= MAX_NB_PROCESSES) {
    return -1;
  }

  // NOTE: p.contexte et p.pile ne sont remplis que partiellement lors de la
  // création du processus. Ils seront complétés par ctx_sw après le premier
  // context switch.
  process_t *p = &scheduler[next_pid];
  p->pid = next_pid;
  p->etat = ACTIVABLE;
  strncpy(p->nom, nom, MAX_CHAR_NAME);

  // On stocke l'adresse de la fonction (l'adresse de la première instruction)
  // dans l'emplacement ra du contexte associé pour que lors du premier appel de
  // ctx_sw, ret saute au début de la fonction.
  p->contexte[0] = (uint64_t)code;

  // À la création, la pile du processus est vide (non initialisée).
  // Le sp du processus est donc juste après la dernière adresse de p->pile car
  // le sommet de la pile est la fin de la zone mémoire allouée.
  p->contexte[1] = (uint64_t)(p->pile + PROCESS_STACK_SIZE);

  next_pid++;
  return p->pid;
}

// Implémente la politique d'ordonnancement en choisissant le prochain processus
// à activer et provoque le changement de processus.
void ordonnance() {
  // WARNING: Il faut exécuter ctx_sw en dernier car ctx_sw modifie les
  // registres et ne les stocke pas sur la pile.
  process_t *next_process = &scheduler[(actif->pid + 1) % next_pid];
  actif->etat = ACTIVABLE;
  next_process->etat = ELU;

  process_t *prev_actif = actif;
  actif = next_process;
  ctx_sw(prev_actif->contexte, next_process->contexte);
}

// Renvoie le pid du processus en cours d'exécution.
int64_t mon_pid() { return actif->pid; }
// Renvoie le nom du processus en cours d'exécution.
char *mon_nom() { return actif->nom; }
