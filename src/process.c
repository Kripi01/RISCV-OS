// TODO: Free les PTE à la mort d'un processus!!!! (grosse memory leak!!!)

#include "process.h"
#include "cpu.h"
#include "interrupt.h"
#include "platform.h"
#include "screen.h"
#include "vm.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int max_nb_pid = 0;
static process_t scheduler[MAX_NB_PROCESSES];
static process_t *actif;
static char *nom_etats[5] = {"ELU", "ACTIVABLE", "ENDORMI", "MORT",
                             "EN ATTENTE"};

// (Ré)initialise la gestion des processus. Idle devient le processus actif.
void init_proc() {
  // PID 0 (idle) -> indispensable pour que le système soit toujours actif
  process_t *p = &scheduler[0];
  p->pid = 0;
  p->pid_parent = 0; // arbitraire car jamais utilisé pour idle
  snprintf(p->nom, MAX_CHAR_NAME, "idle");
  p->etat = ELU;
  p->contexte[0] = (uint64_t)idle;
  p->contexte[1] = (uint64_t)(p->pile + PROCESS_STACK_SIZE - 1);

  // Initialise la mémoire virtuelle (satp)
  uint64_t satp = init_vm(p->pid);
  p->satp = satp;
  p->pile[PROCESS_STACK_SIZE - 1] = satp; // satp est aussi sur la pile

  // idle devient le processus courant
  actif = p;
  max_nb_pid = 1;

  // On bascule directement l'adressage (pagination active)
  __asm__("csrw satp, %0" ::"r"(satp));
  __asm__("sfence.vma zero, zero");
}

// Crée un processus, sans l'élire, et renvoie son PID (ou -1 si erreur).
// Ne gère pas la filiation (cf. waitpid)
int64_t cree_processus(int code(), char *nom) {
  if (max_nb_pid >= MAX_NB_PROCESSES) {
    return -1;
  }

  // On récupère PID disponible (de processus MORT ou bien un nouveau PID)
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
  // snprintf et pas strncpy pour garantir le caractère de terminaison '\0'
  snprintf(p->nom, MAX_CHAR_NAME, "%s", nom);

  // pour le saut lors du premier context switch (p->contexte[0] == ra)
  p->contexte[0] = (uint64_t)proc_launcher;

  // On stocke l'adresse du processus et satp sur la pile (p->contexte[1] == sp)
  p->contexte[1] = (uint64_t)(p->pile + (PROCESS_STACK_SIZE - 2));

  p->pile[PROCESS_STACK_SIZE - 2] = (uint64_t)code;
  uint64_t satp = init_vm(p->pid); // on utilise le PID pour l'ASID (unicité)
  p->satp = satp;
  p->pile[PROCESS_STACK_SIZE - 1] = satp;
  // On bascule l'adressage lors du context switch
  // NOTE: p->contexte et p->pile ne sont remplis que partiellement lors de la
  // création du processus. Ils seront complétés par ctx_sw.

  return p->pid;
}

// Sélectionne le prochain processus (via la politique de Round-robin) et
// provoque le changement de contexte.
void ordonnance() {
  uint64_t next_idx = actif->pid + 1;
  process_t *next_process = &scheduler[next_idx % max_nb_pid];

  // On parcourt le scheduler pour trouver le prochain processus activable (il y
  // a toujours au moins idle). On ignore les processus morts, en attente ou
  // endormis (si l'heure de réveil n'est pas atteinte)
  uint64_t n = nbr_secondes();
  while (next_process->etat == MORT || next_process->etat == EN_ATTENTE ||
         (next_process->etat == ENDORMI && next_process->heure_reveil > n)) {
    next_idx++;
    next_process = &scheduler[next_idx % max_nb_pid];
  }

  // L'ancien processus devient activable SSI il était élu (sinon il ne bouge
  // pas) et le nouveau devient élu
  actif->etat = actif->etat == ELU ? ACTIVABLE : actif->etat;
  next_process->etat = ELU;

  process_t *prev_actif = actif;
  actif = next_process;

  affiche_etats();

  // Il faut exécuter ctx_sw en dernier car ctx_sw modifie les
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

// Kill le processus actif et réactive son parent (sans forcément l'élire)
// WARNING: Cette fonction ne doit jamais être appelée sur idle.
void fin_processus() {
  // TODO: Vider la mémoire occupée par le processus
  actif->etat = MORT;

  process_t *parent = (&scheduler[actif->pid_parent]);
  // Si le parent est mort entre temps alors on ne le réanime pas!
  if (parent->etat == EN_ATTENTE) {
    (&scheduler[actif->pid_parent])->etat = ACTIVABLE; // on réactive le parent
  }

  ordonnance();
}

// Met le processus actif en attente de la mort d'un fils spécifique (PID=pid)
void waitpid(int64_t pid) {
  process_t *p = &scheduler[pid];

  actif->etat = EN_ATTENTE;
  p->pid_parent = actif->pid;

  ordonnance(); // le prochain prorcessus n'est pas forcément le fils
}

// Affiche l'état de chaque processus en haut à gauche de l'écran (élu == vert)
void affiche_etats() {
  const uint64_t len_buffer = 115; // pour ne pas écraser l'heure (max = 118)

  // On vide la ligne d'abord
  for (uint64_t i = 0; i < FONT_HEIGHT; i++) {
    memset((uint32_t *)(BOCHS_DISPLAY_BASE_ADDRESS +
                        i * DISPLAY_WIDTH * BYTES_PER_PIXEL),
           BLACK, BYTES_PER_PIXEL * len_buffer * FONT_WIDTH);
  }

  // Puis on affiche la chaîne de caractères (nom + PID + état)
  uint64_t col = 0;
  for (int i = 0; i < max_nb_pid; i++) {
    process_t *p = &scheduler[i];
    // On n'affiche pas les processus morts
    if (p->etat != MORT) {
      char tmp[len_buffer];
      int n = snprintf(tmp, len_buffer, "%s(PID=%ld):%s ", p->nom, p->pid,
                       nom_etats[p->etat]);

      for (int j = 0; j < n && col < len_buffer; j++) {
        ecrit_car(0, col, tmp[j], p->etat == ELU ? GREEN : WHITE, BLACK);
        col++;
      }
    }
  }
}

// Lance le processus en argument et s'assure de la terminaison de celui-ci.
void proc_launcher(int proc()) {
  // Les nouveaux processus commencent avec les interruptions timer autorisées.
  // Ça fixe le bug de non-actualisation de l'horloge quand un processus est
  // lancé par un processus ayant les interruptions désactivées e.g. bash
  s_enable_it();

  proc();

  // NOTE: idle ne termine jamais donc il n'atteint jamais cette ligne
  fin_processus();
}

// Commande ps: affiche les processus actifs sous la forme d'un tableau.
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
