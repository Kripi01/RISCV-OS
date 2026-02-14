#include "process.h"
#include "cpu.h"
#include "interrupt.h"
#include "platform.h"
#include "screen.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int max_nb_pid = 0;
static process_t scheduler[MAX_NB_PROCESSES];
static process_t *actif;
static char *nom_etats[5] = {"ELU", "ACTIVABLE", "ENDORMI", "MORT",
                             "EN ATTENTE"};

// Initialise (ou de réinitialise) le système pour la gestion des processus.
// Cette fonction ne démarre pas de nouveau processus.
void init_proc() {
  // Le processus de pid 0 est toujours nommé idle. Par défaut, c'est lui qui
  // est actif car dans un système, il doit toujours y avoir au moins un
  // processus actif, car le système ne doit jamais s'arrêter.
  process_t *p = &scheduler[0];
  p->pid = 0;
  p->pid_parent = 0; // arbitraire car jamais utilisé pour idle
  snprintf(p->nom, MAX_CHAR_NAME, "idle");
  p->etat = ELU;
  p->contexte[0] = (uint64_t)idle;
  p->contexte[1] = (uint64_t)(p->pile + PROCESS_STACK_SIZE);

  actif = p;
  max_nb_pid = 1;
}

// Renvoie le pid du processus créé, ou -1 en cas d'erreur. Ne gère pas la
// filiation du processus (il faut appeler waitpid)
int64_t cree_processus(int code(), char *nom) {
  if (max_nb_pid >= MAX_NB_PROCESSES) {
    return -1;
  }

  // On prend un PID de processus MORT ou un nouveau PID si aucun processus mort
  // (dans ce cas, on incrémente le nombre max de PID "en circulation")
  int new_pid = -1;
  for (int i = 0; i < max_nb_pid; i++) {
    if ((&scheduler[i])->etat == MORT) {
      new_pid = i;
      break;
    }
  }

  if (new_pid == -1) {
    new_pid = max_nb_pid;
    max_nb_pid++;
  }

  process_t *p = &scheduler[new_pid];
  p->pid = new_pid;
  p->etat = ACTIVABLE;
  // On utilise snprintf et pas strncpy car on veut le caractère de terminaison
  // '\0' pour afficher l'état des processus ensuite.
  snprintf(p->nom, MAX_CHAR_NAME, "%s", nom);

  // NOTE: p->contexte et p->pile ne sont remplis que partiellement lors de la
  // création du processus. Ils seront complétés par ctx_sw après le premier
  // context switch.

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

  return p->pid;
}

// Implémente la politique d'ordonnancement en choisissant le prochain processus
// à activer et provoque le changement de processus.
void ordonnance() {
  uint64_t next_idx = actif->pid + 1;
  process_t *next_process = &scheduler[next_idx % max_nb_pid];
  // On trouve le prochain processus (idle vérifie toujours ces conditions)
  // L'heure de réveil est correcte que lorsque le processus est endormi. Sinon,
  // elle est périmée.
  uint64_t n = nbr_secondes();
  while (next_process->etat == MORT || next_process->etat == EN_ATTENTE ||
         (next_process->etat == ENDORMI && next_process->heure_reveil > n)) {
    next_idx++;
    next_process = &scheduler[next_idx % max_nb_pid];
  }

  // Si l'état actif n'a pas été endormi ou tué ou en_attente, alors on le met
  // en activable, sinon on le laisse endormi ou tué ou en_attente
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

// Kill le processus actif et remet son parent en activable (s'il n'a pas changé
// d'état depuis). Enfin, change de processus (ne redonne pas forcément la main
// au processus parent).
// WARNING: Cette fonction ne doit jamais être appelée sur idle.
void fin_processus() {
  actif->etat = MORT;

  process_t *parent = (&scheduler[actif->pid_parent]);
  // Si le parent est mort entre temps alors on ne le réanime pas!
  if (parent->etat == EN_ATTENTE) {
    // Le processus parent est de nouveau activable.
    (&scheduler[actif->pid_parent])->etat = ACTIVABLE;
  }

  ordonnance();
}

// Le processus actif passe en attente jusqu'à ce que le processus fils
// (PID=pid) soit mort (cf. fin_processus). On change donc de processus (on ne
// passe pas nécessairement au processus fils)
void waitpid(int64_t pid) {
  process_t *p = &scheduler[pid];

  actif->etat = EN_ATTENTE;
  p->pid_parent = actif->pid;

  ordonnance();
}

// Affiche l'état de chaque processus en haut à gauche de l'écran
void affiche_etats() {
  // La taille du buffer est choisie pour que ça n'écrase pas l'heure (le max
  // sans écraser étant 118)
  const uint64_t len_buffer = 115;

  // On vide la ligne d'abord
  for (uint64_t i = 0; i < FONT_HEIGHT; i++) {
    memset((uint32_t *)(BOCHS_DISPLAY_BASE_ADDRESS +
                        i * DISPLAY_WIDTH * BYTES_PER_PIXEL),
           BLACK, BYTES_PER_PIXEL * len_buffer * FONT_WIDTH);
  }

  char s[len_buffer];
  uint64_t pos = 0;
  for (int i = 0; i < max_nb_pid; i++) {
    process_t *p = &scheduler[i];
    // On n'affiche pas les processus morts
    if (p->etat != MORT) {
      int remaining = len_buffer - pos;
      pos += snprintf(s + pos, remaining, "%s(PID=%ld):%s ", p->nom, p->pid,
                      nom_etats[p->etat]);
    }
  }

  for (uint64_t i = 0; i < pos; i++) {
    ecrit_car(0, i, s[i], WHITE, BLACK);
  }
}

// Lance le processus en argument et s'assure de la terminaison de celui-ci à la
// fin de son exécution.
// NOTE: Comme idle ne termine jamais, on arrive jamais à l'instruction
// fin_processus, donc idle n'est jamais killed.
void proc_launcher(int proc()) {
  // On force les nouveaux processus à commencer avec les interruptions timer
  // autorisées (ils peuvent toujours les désactiver ensuite). Ça fixe le bug de
  // non-actualisation de l'horloge quand un processus est lancé par un
  // processus ayant les interruptions désactivées (bash par exemple).
  s_enable_it();

  proc();
  fin_processus();
}

int ps() {
  printf("+-----------------+-------+------------+\n");
  printf("| Nom             | PID   | Etat       |\n");
  printf("|-----------------+-------+------------|\n");
  for (int i = 0; i < max_nb_pid; i++) {
    process_t *p = &scheduler[i];
    // On n'affiche pas les processus morts
    if (p->etat != MORT) {
      printf("| %-15s | %-5ld | %-10s |\n", p->nom, p->pid, nom_etats[p->etat]);
    }
  }
  printf("+-----------------+-------+------------+\n");

  return 0;
}
