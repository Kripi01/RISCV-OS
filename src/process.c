#include "process.h"
#include "interrupt.h"
#include "platform.h"
#include "screen.h"
#include <stdint.h>
#include <stdio.h>
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
  snprintf(p->nom, MAX_CHAR_NAME, "idle");
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
  // On utilise snprintf et pas strncpy car on veut le caractère de terminaison
  // '\0' pour afficher l'état des processus ensuite.
  snprintf(p->nom, MAX_CHAR_NAME, "%s", nom);

  // On stocke l'adresse de proc_launcher (l'adresse de la première instruction)
  // dans l'emplacement ra du contexte associé pour que lors du premier appel de
  // ctx_sw, ret saute au début de la fonction.
  p->contexte[0] = (uint64_t)proc_launcher;

  // Il faut stocker l'argument de proc_launcher dans la pile pour que ctx_sw
  // puisse appeler proc_launcher avec le bon argument. À la création, la pile
  // du processus est quasi vide. Le sp du processus est donc
  // à la dernière adresse de p->pile car le sommet de la pile est la
  // fin de la zone mémoire allouée et on stocke seulement l'argument de
  // proc_launcher
  p->contexte[1] = (uint64_t)(p->pile + (PROCESS_STACK_SIZE - 1));
  p->pile[PROCESS_STACK_SIZE - 1] = (uint64_t)code;

  next_pid++;
  return p->pid;
}

// Implémente la politique d'ordonnancement en choisissant le prochain processus
// à activer et provoque le changement de processus.
void ordonnance() {
  uint64_t next_idx = actif->pid + 1;
  process_t *next_process = &scheduler[next_idx % next_pid];
  // On trouve le prochain processus (idle vérifie toujours ces conditions)
  // L'heure de réveil est correcte que lorsque le processus est endormi. Sinon,
  // elle est périmée.
  uint64_t n = nbr_secondes();
  while (next_process->etat == MORT ||
         (next_process->etat == ENDORMI && next_process->heure_reveil > n)) {
    next_idx++;
    next_process = &scheduler[next_idx % next_pid];
  }

  // Si l'état actif n'a pas été endormi ou tué, alors on le met en activable,
  // sinon on le laisse endormi ou tué
  actif->etat = actif->etat == ELU ? ACTIVABLE : actif->etat;
  next_process->etat = ELU;

  process_t *prev_actif = actif;
  actif = next_process;

  affiche_etats();

  // WARNING: Il faut exécuter ctx_sw en dernier car ctx_sw modifie les
  // registres et ne les stocke pas sur la pile.
  ctx_sw(prev_actif->contexte, next_process->contexte);
}

// Renvoie le pid du processus en cours d'exécution.
int64_t mon_pid() { return actif->pid; }
// Renvoie le nom du processus en cours d'exécution.
char *mon_nom() { return actif->nom; }

// Endort le processus actif et change de processus.
// WARNING: Cette fonction ne doit jamais être appelée sur idle.
void dors(uint64_t nbr_secs) {
  actif->etat = ENDORMI;
  actif->heure_reveil = nbr_secondes() + nbr_secs;
  ordonnance();
}

// Kill le processus actif et change de processus.
// WARNING: Cette fonction ne doit jamais être appelée sur idle.
void fin_processus() {
  actif->etat = MORT;
  ordonnance();
}

// Affiche l'état de chaque processus en haut à gauche de l'écran
void affiche_etats() {
  const uint64_t len_buffer = 100;

  // On vide la ligne d'abord
  for (uint64_t i = 0; i < FONT_HEIGHT; i++) {
    memset((uint32_t *)(BOCHS_DISPLAY_BASE_ADDRESS +
                        i * DISPLAY_WIDTH * BYTES_PER_PIXEL),
           BLACK, BYTES_PER_PIXEL * len_buffer * FONT_WIDTH);
  }

  char *nom_etats[] = {"ELU", "ACTIVABLE", "ENDORMI", "MORT"};

  char s[200];
  int pos = 0;
  for (int i = 0; i < next_pid; i++) {
    process_t *p = &scheduler[i];
    pos += sprintf(s + pos, "%s:%s ", p->nom, nom_etats[p->etat]);
  }

  for (int i = 0; i < pos; i++) {
    ecrit_car(0, i, s[i], WHITE, BLACK);
  }
}

// Lance le processus en argument et s'assure de la terminaison de celui-ci à la
// fin de son exécution.
// NOTE: Comme idle ne termine jamais, on arrive jamais à l'instruction
// fin_processus, donc idle n'est jamais killed.
void proc_launcher(void proc()) {
  proc();
  fin_processus();
}
