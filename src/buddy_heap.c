// Sources: https://en.wikipedia.org/wiki/Buddy_memory_allocation
// https://en.wikipedia.org/wiki/Free_list

// Le tas est représenté par un tableau de free lists. L'élément d'indice i du
// tableau contient une free_list de blocs de taille 2^i (header inclu).

#include "buddy_heap.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern char _heap_start;
const uintptr_t heap_start = (uintptr_t)&_heap_start;
extern char _heap_end;
const uintptr_t heap_end = (uintptr_t)&_heap_end;
static size_t heap_size;

// Il faut que l'ordre minimal soit strictement supérieur à la taille en
// bytes du header (=32) parce que sinon les blocs overlappent
static const size_t min_order = 5;

static uint8_t biggest_order;
// On a au MAXIMUM 64 ordres (car adresses sur 64 bits)
static buddy_free_list_t *heap_arr[MAX_ORDER];

// Retourne la partie supérieure de log2(x) (= le plus haut bit à 1 de x)
static uint8_t integer_log2(uint64_t x) {
  if (x <= 1) {
    return 0; // Par commodité, on pose log2(0) = 0
  }

  // On cherche le plus haut bit valant 1 dans x-1 (pour éviter le pb de la
  // puissance de deux exacte)
  x--;
  uint8_t i = 0;
  while (x != 0) {
    x >>= 1;
    i++;
  }

  return i;
}

// Initialise le tas: heap_size, biggest_order, heap_arr et les free lists
void buddy_init_heap() {
  heap_size = heap_end - heap_start;
  memset(heap_arr, 0, sizeof(heap_arr)); // remise à zero des free list
  // On décompose heap_size en puissances de 2 et on crée un bloc pour chacune
  uintptr_t heap_ptr = heap_start;
  biggest_order = 0;
  // NOTE: la boucle doit se faire à l'envers car un bloc de taille 2^k doit
  // toujours commencer à une adresse multiple de 2^k (tous les bits d'indice
  // inférieur à k sont à 0) pour que le XOR du buddy fonctionne.
  for (int order = MAX_ORDER - 1; order >= (int)min_order; order--) {
    uint64_t bit = (heap_size >> order) & 1;
    if (bit == 1) {
      if (order > biggest_order) {
        biggest_order = order;
      }
      buddy_free_list_t block = {
          .order = order, .is_free = 1, .prev = NULL, .next = NULL};
      *(buddy_free_list_t *)heap_ptr = block;
      heap_arr[order] = (buddy_free_list_t *)heap_ptr;
      heap_ptr += (1ULL << order);
    }
  }
}

// Ajoute le block au début de la free list d'ordre order
inline static void list_prepend(uint8_t order, buddy_free_list_t *block) {
  if (block == NULL) {
    return;
  }

  block->is_free = 1;
  block->order = order;
  block->prev = NULL;
  block->next = heap_arr[order];
  if (heap_arr[order] != NULL) {
    heap_arr[order]->prev = block;
  }
  heap_arr[order] = block;
}

// Extrait le block de la free list
inline static void list_remove(uint8_t order, buddy_free_list_t *block) {
  if (block == NULL) {
    return;
  }

  block->is_free = 0;
  if (block->next != NULL) {
    block->next->prev = block->prev;
  }
  if (block->prev != NULL) {
    block->prev->next = block->next;
  } else {
    // Si le précédent est NULL alors le block est au début de la free list
    heap_arr[order] = block->next;
  }
}

// Coupe un bloc libre en deux blocs d'ordre inférieur (utile pour malloc)
static void split_free_block(uint8_t order, buddy_free_list_t *block) {
  if (order == 0) {
    return; // On ne peut pas split un bloc d'ordre 0
  }

  // On supprime le bloc dans la free list originale
  list_remove(order, block);

  // Puis on ajoute deux blocs à la free list d'ordre inférieur
  uint8_t new_order = order - 1;
  uint64_t new_block_size = 1ULL << new_order;

  buddy_free_list_t *new_block1 = block;
  buddy_free_list_t *new_block2 =
      (buddy_free_list_t *)((uintptr_t)new_block1 + new_block_size);
  list_prepend(new_order, new_block2);
  list_prepend(new_order, new_block1);
}

// Alloue un bloc de mémoire de taille memorySize octets dans le tas.
void *buddy_malloc(size_t memorySize) {
  uint8_t order = integer_log2(memorySize + FL_REDUCED_HEADER_SIZE);

  if (order > biggest_order) {
    printf("Error buddy_malloc: Allocation dynamique trop importante.\n");
    return NULL;
  }

  // On cherche un bloc de taille 2^order
  buddy_free_list_t *free_bloc = heap_arr[order];
  if (free_bloc != NULL) { // on prend le premier bloc libre
    // on supprime le bloc de la free list
    list_remove(order, free_bloc);

    // On enlève le header réduit, i.e. le is_free et l'order
    return (void *)((uintptr_t)free_bloc + FL_REDUCED_HEADER_SIZE);
  }

  // Si on ne l'a pas trouvé alors on le crée en splittant des blocs plus grands
  uint8_t k = order + 1;
  while (k != order) {
    // On prend le premier bloc libre d'ordre strictement supérieur à order
    while (k <= biggest_order && heap_arr[k] == NULL) {
      k++;
    }

    if (k > biggest_order) {
      return NULL; // le tas est plein
    }

    // On splitte ce bloc
    split_free_block(k, heap_arr[k]);
    k--; // l'ordre le plus proche de order a diminué de 1 car on a créé de
    // nouveaux blocs d'ordre inférieur
  }

  // On retourne le bloc (sans header) après l'avoir retiré de la free list
  buddy_free_list_t *block_found = heap_arr[order];
  void *allocated_block =
      (void *)((uintptr_t)block_found + FL_REDUCED_HEADER_SIZE);
  list_remove(order, block_found);
  return allocated_block;
}

// Libère le pointeur alloué dynamiquement sur le tas
void buddy_free(void *ptr) {
  if (ptr == NULL) {
    printf("Error buddy_free: Tentative de free NULL\n");
    return;
  }

  if ((uintptr_t)ptr < heap_start || (uintptr_t)ptr > heap_end) {
    printf("Error buddy_free: Tentative de free un pointeur qui n'est pas sur "
           "le tas\n");
    return;
  }

  buddy_free_list_t *block =
      (buddy_free_list_t *)((uintptr_t)ptr - FL_REDUCED_HEADER_SIZE);
  if (block->is_free == 1) {
    return;
  }
  block->is_free = 1;

  uint8_t order = block->order;
  // Si la fusion est impossible dès le début, le premier 'if' dans le while
  // fera un 'break' et on ira directement à list_prepend à la fin (c'est le cas
  // où on ne peut pas coalescer).
  while (order < biggest_order) {
    // La formule du buddy fonctionne si les adresses du tas commencent à 0
    uintptr_t buddy_addr = heap_start + (((uintptr_t)block - heap_start) ^
                                         (uintptr_t)(1ULL << order));
    buddy_free_list_t *buddy = (buddy_free_list_t *)buddy_addr;

    if (buddy_addr >= heap_end || buddy->is_free == 0 ||
        buddy->order != order) {
      break;
    }

    // On retire le buddy (fusion intermédiaire)
    list_remove(order, buddy);

    // Si le buddy est à gauche alors c'est lui qui devient le nouveau gros bloc
    if (buddy_addr < (uintptr_t)block) {
      block = buddy;
    }

    // On finalise la fusion intermédiaire
    order++;
    block->order = order;
  }

  list_prepend(order, block);
}

// Alloue un bloc de mémoire dans le tas, de taille elementCount éléments de
// taille elementSize, et initialisé à 0.
void *buddy_calloc(size_t elementCount, size_t elementSize) {
  size_t total_size = elementCount * elementSize;
  void *ptr = buddy_malloc(total_size);
  if (ptr == NULL) {
    return NULL;
  }

  memset(ptr, 0, total_size);
  return ptr;
}

// Réalloue le bloc de mémoire à l'adresse pointer, de taille memorySize dans le
// tas, en l'agrandissant ou le rétressissant. Retourne l'adresse du nouveau
// bloc alloué
void *buddy_realloc(void *pointer, size_t memorySize) {
  if (pointer == NULL) {
    // C'est un malloc classique
    return buddy_malloc(memorySize);
  }

  if (memorySize == 0) {
    // C'est un free
    buddy_free(pointer);
    return NULL;
  }

  buddy_free_list_t *block =
      (buddy_free_list_t *)((uintptr_t)pointer - FL_REDUCED_HEADER_SIZE);
  uint8_t order = block->order;
  size_t cur_size = (1ULL << order) - FL_REDUCED_HEADER_SIZE;

  if (memorySize <= cur_size) {
    // Si la nouvelle taille rentre dans le bloc actuel alors on ne fait rien
    return pointer;
  }

  // On alloue un nouveau bloc
  // PERF: ce n'est pas optimal car alloue un tout nouveau bloc au lieu
  // d'essayer de coalescer avec le buddy (s'il est libre), mais c'est plus
  // compliqué à coder)
  void *new_ptr = buddy_malloc(memorySize);
  if (new_ptr == NULL) {
    printf("buddy_realloc Error\n");
    return NULL;
  }

  // On copie les données de l'ancien vers le nouveau
  // Si le realloc veut tronquer les données alors on tronque
  size_t new_size = (memorySize < cur_size) ? memorySize : cur_size;
  memcpy(new_ptr, pointer, new_size);

  buddy_free(pointer);

  return new_ptr;
}
