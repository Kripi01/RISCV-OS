// TODO: Free les PTE à la mort d'un processus!!!! (grosse memory leak!!!)
// Il faut aussi free le code qui a été copié du kernel vers la zone mémoire du
// processus (et mettre toute la mémoire à zero par sécurité).
// TODO: repositionner les fonctions (refactor)
// TODO: Ajouter un champ priorité aux processus pour améliorer la politique
// d'ordonnacement (exemple: idle a une prorité de 0 et bash une priorité de 5)

#include "process.h"
#include "cpu.h"
#include "interrupt.h"
#include "platform.h"
#include "pm.h"
#include "screen.h"
#include "uheap.h"
#include "vm.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int max_nb_pid = 0;
static process_t scheduler[MAX_NB_PROCESSES];
static process_t *actif;
static char *nom_etats[5] = {"ELU", "ACTIVABLE", "ENDORMI", "MORT",
                             "EN ATTENTE"};

// (Ré)initialise la gestion des processus. Idle devient le processus actif
// idle est indispensable pour que le système soit toujours actif.
void init_proc() {
  process_t *p = &scheduler[0];
  p->pid = 0;        // idle est toujours de PID 0
  p->pid_parent = 0; // arbitraire car jamais utilisé pour idle
  snprintf(p->nom, MAX_CHAR_NAME, "idle");
  p->etat = ELU;

  p->contexte[1] = (uint64_t)(p->pile + PROCESS_STACK_SIZE);
  // idle est exécuté en mode S, pas de pile user -> toujours le même sp donc on
  // ne met rien dans sscratch
  __asm__("csrw sscratch, zero");

  p->contexte[16] = (uint64_t)idle;             // sepc
  p->contexte[17] = SSTATUS_SPP | SSTATUS_SPIE; // mode S et IRQ à autoriser

  uint64_t satp = init_vm(p->pid); // on initialise la mémoire virtuelle (satp)
  p->contexte[18] = satp;

  // idle devient le processus courant
  actif = p;
  max_nb_pid = 1;

  // NOTE: on alloue pas de tas pour idle

  // On bascule directement l'adressage (pagination active)
  __asm__("csrw satp, %0" ::"r"(satp));
  __asm__("sfence.vma zero, zero");
}

// Alloue + mappe la pile pour le processus fils et copie les arguments (argc et
// argv) sur la pile user.
static void init_user_stack(process_t *p, pagetable_t root, int argc,
                            char **argv) {
  // La pile user n'est pas fiable pour le mode S, donc on utilise une pile
  // différente (pour l'ordonnacement, interruptions, etc), le sommet (le sp)
  // de la pile kernel est dans sscratch et p->pile.
  void *pa_stack = get_frame();
  memset(pa_stack, 0, FRAMESIZE); // par sécurité
  // la pile user va de 0x4FFFF000 à 0x50000000
  map_page(root, USER_STACK_ADDRESS - PAGESIZE, (uintptr_t)pa_stack,
           PTE_RWXV | PTE_U);

  // On place dans la pile user les chaînes de caractères des arguments, puis le
  // tableau des adresses: new_argv. (sinon, les arguments sont des adresses sur
  // la pile du processus père: e.g. bash)
  enable_sum();

  // On copie les arguments sur la pile du processus.
  uintptr_t sp = USER_STACK_ADDRESS;
  uintptr_t ksp = (uintptr_t)pa_stack + FRAMESIZE; // pa de la pile user
  char *new_argv[argc];
  for (int i = 0; i < argc; i++) {
    // on décrémente AVANT de copier car la pile "descend"
    sp -= (strlen(argv[i]) + 1) * sizeof(char); // on inclut le '\0' de fin
    ksp -= (strlen(argv[i]) + 1) * sizeof(char);

    // WARNING: on est dans le satp du père, donc les adresses virtuelles ne
    // sont pas valides pour le fils. On écrit donc directement dans la mémoire
    // physique kernel (via ksp).
    strcpy((char *)ksp, argv[i]);
    new_argv[i] = (char *)sp;
  }

  // On copie également sur la pile user, le tableau des nouvelles adresses vers
  // les arguments copiés: new_argv
  size_t argv_len = argc * sizeof(char *);
  sp -= argv_len;  // on inclut le '\0' de fin
  ksp -= argv_len; // on inclut le '\0' de fin
  memcpy((char **)ksp, new_argv, argv_len);

  // NOTE: pas besoin de copier argc car il n'y a pas de pb d'adresses (c'est
  // juste un entier), donc on le passe directement dans proc_launcher

  disable_sum();

  // On stocke les 2 arguments du processus dans s0 et s1 (pour proc_launcher)
  p->contexte[4] = argc;         // s0
  p->contexte[5] = (uint64_t)sp; // s1 (on stocke la va du FILS)

  // On stocke le sp du processus
  p->contexte[6] = sp; // s2
}

// Charge le processus proc (adresse possiblement virtuelle), de taille max
// proc_size, dans une zone mémoire accessible au processus user (p).
// L'adresse du code du nouveau processus est placé dans le champ sepc de son
// contexte (un peu après USER_START_ADDR = 0x40000000)
// WARNING: load_process doit être appelé avec le satp du processus père (on
// l'appelle dans cree_processus donc OK)
static void load_process(process_t *p, uintptr_t proc_addr, size_t proc_size,
                         int argc, char **argv) {
  uint64_t satp = p->contexte[18];
  pagetable_t root = SATP2PT(satp);

  // On copie + mappe le plus petit ensemble de pages contenant le code du
  // processus dans une zone mémoire accessible au mode user (à partir de
  // USER_START_ADDR).
  // NOTE: Le code du processus est donc à 2 endroits (kernel et zone user)
  uintptr_t page_start = proc_addr & ~(PAGESIZE - 1);
  uintptr_t offset_in_page = proc_addr & (PAGESIZE - 1);

  size_t total_to_copy = proc_size + offset_in_page;
  size_t copied = 0;
  while (copied < total_to_copy) {
    void *pa = get_frame();
    memcpy((void *)pa, (void *)((uintptr_t)page_start + copied), PAGESIZE);
    map_page(root, USER_START_ADDR + copied, (uintptr_t)pa, PTE_RWXV | PTE_U);

    copied += PAGESIZE;
  }
  // WARNING: Le début du code du nouveau processus est donc USER_START_ADDR +
  // offset_in_page car on a copié toute la page
  p->contexte[16] = USER_START_ADDR + offset_in_page; // sepc

  init_user_stack(p, root, argc, argv);
}

// Crée et alloue la mémoire d'un processus, sans l'élire, et renvoie son PID
// (ou -1 si erreur). Ne gère pas la filiation (cf. waitpid)
// WARNING: La taille maximale du processus est 4KB (ajustable)
int64_t cree_processus(int code(int, char **), int argc, char **argv) {
  if (max_nb_pid >= MAX_NB_PROCESSES) {
    return -1;
  }

  // On récupère un PID disponible (de processus MORT ou bien un nouveau PID)
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

  enable_sum(); // car argv est une adresse user
  // le nom du processus est le premier argument.
  snprintf(p->nom, MAX_CHAR_NAME, "%s", argv[0]);
  disable_sum();

  // Lors du premier ctx_sw, le ret nous emmène dans proc_launcher
  p->contexte[0] = (uint64_t)proc_launcher; // ra

  p->contexte[1] = (uint64_t)(p->pile + PROCESS_STACK_SIZE); // pile kernel

  // Au premier ctx_sw, on reste en mode S car on doit apeler proc_launcher.
  p->contexte[17] = SSTATUS_SPP | SSTATUS_SPIE; // mode S et IRQ à autoriser

  uint64_t satp = init_vm(p->pid); // on utilise le PID pour l'ASID (unicité)
  p->contexte[18] = satp; // On basculera l'adressage lors du context switch

  // On copie + mappe le code du processus dans sa mémoire (taille max = 40KB).
  // On alloue également la pile user et on y stocke les arguments du processus.
  load_process(p, (uintptr_t)code, 10 * PAGESIZE, argc, argv);

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
  const uint64_t len_buffer = 118; // pour ne pas écraser l'heure (max = 118)

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

// Lance le processus actif en mode U et s'assure de la terminaison de celui-ci.
// WARNING: proc_launcher doit être appelé avec le satp du processus fils (le
// changement de satp est fait dans ctx_sw)
void proc_launcher() {
  init_uheap();

  // On passe en mode user (après le sret)
  __asm__("csrc sstatus, %0" ::"r"(SSTATUS_SPP));  // mode user
  __asm__("csrs sstatus, %0" ::"r"(SSTATUS_SPIE)); // IRQ seront autorisées

  // On sauvegarde le sp kernel dans sscratch
  uintptr_t kstack_top = (uintptr_t)actif->pile + PROCESS_STACK_SIZE;
  __asm__("csrw sscratch, %0" ::"r"(kstack_top));

  // on charge la pile user
  __asm__("mv sp, %0" ::"r"(actif->contexte[6])); // cf. init_user_stack

  // On charge l'adresse du processus et les bons arguments
  __asm__("csrw sepc, %0" ::"r"(actif->contexte[16]));
  __asm__("mv a0, %0" ::"r"(actif->contexte[4])); // argc
  __asm__("mv a1, %0" ::"r"(actif->contexte[5])); // argv

  // au moment du sret, sstatus.SPIE est recopié dans sstatus.SIE et les
  // interruptions seront autorisées -> l'horloge continuera à s'actualiser
  __asm__("sret"); // on jump directement vers le code du processus (=sepc)
  // NOTE: Pour terminer correctement le processus, il faut faire le syscall
  // UEXIT (qui appelle fin_processus) à la fin.
}

// Commande ps: affiche les processus actifs sous la forme d'un tableau.
// WARNING: Doit être exécuté en mode S.
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

/* ======================================================= */
/* ============== GESTION DU TAS DES PROCESSUS =========== */
/* ======================================================= */

// WARNING: Cette fonction doit être exécutée en mode S
static void allocate_heap_frames() {
  uint64_t satp = actif->contexte[18];
  pagetable_t root = SATP2PT(satp);

  for (uint64_t page = 0; page < HEAP_SIZE; page += PAGESIZE) {
    void *pa = get_frame();
    map_page(root, HEAP_START + page, (uintptr_t)pa, PTE_RWV | PTE_U);
  }

  // le heap_arr est stocké juste avant le début du tas (la page précédente). Il
  // fait 64 * 8 = 512 bytes donc il tient sur une page
  void *pa = get_frame();
  map_page(root, HEAP_ARR_START, (uintptr_t)pa, PTE_RWXV | PTE_U);
  memset((void *)HEAP_ARR_START, 0, MAX_ORDER); // remise à zero des free list
}

// Initialise le tas du processus (en mémoire user): HEAP_ARR et les free lists.
// WARNING: Cette fonction doit être exécutée en mode S
void init_uheap() {
  enable_sum();
  allocate_heap_frames();

  // On décompose heap_size en puissances de 2 et on crée un bloc pour chacune
  uintptr_t heap_ptr = HEAP_START;
  // NOTE: la boucle doit se faire à l'envers car un bloc de taille 2^k doit
  // toujours commencer à une adresse multiple de 2^k (tous les bits d'indice
  // inférieur à k sont à 0) pour que le XOR du buddy fonctionne.
  for (int order = MAX_ORDER - 1; order >= (int)MIN_ORDER; order--) {
    uint64_t bit = (HEAP_SIZE >> order) & 1;
    if (bit != 0) {
      heap_fl_t block = {
          .order = order, .is_free = 1, .prev = NULL, .next = NULL};
      *(heap_fl_t *)heap_ptr = block;
      ((heap_fl_t **)HEAP_ARR_START)[order] = (heap_fl_t *)heap_ptr;
      heap_ptr += (1ULL << order);
    }
  }

  disable_sum();
}
