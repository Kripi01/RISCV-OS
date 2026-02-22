// Sources: https://en.wikipedia.org/wiki/Buddy_memory_allocation
// https://en.wikipedia.org/wiki/Free_list

// ======================================= //
// ============= TAS DU KERNEL =========== //
// ==== VIA UN BUDDY MEMORY ALLOCATOR ==== //
// ======================================= //

// Le tas du kernel (utilisé notamment dans ramfs), donc toutes les fonctions
// doivent être exécutées en mode S. Les adresses sont toutes physiques (le code
// n'utilise pas l'allocateur de frames, la mémoire du tas est gérée finement
// par le buddy allocator). Pour le tas des processus, regarder uheap.c

// Le tas est représenté par un tableau de free lists. L'élément d'indice i du
// tableau contient une free_list de blocs de taille 2^i (header inclu).

#include "kheap.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern char _kheap_start;
const uintptr_t kheap_start = (uintptr_t)&_kheap_start;
extern char _kheap_end;
const uintptr_t kheap_end = (uintptr_t)&_kheap_end;
static size_t kheap_size;

static uint8_t biggest_order;
// On a au MAXIMUM 64 ordres (car adresses sur 64 bits)
static heap_fl_t *kheap_arr[MAX_ORDER];

// Initialise le tas: kheap_size, biggest_order, kheap_arr et les free lists
void init_kheap() {
  kheap_size = kheap_end - kheap_start;
  memset(kheap_arr, 0, sizeof(kheap_arr)); // remise à zero des free lists
  // On décompose heap_size en puissances de 2 et on crée un bloc pour chacune
  uintptr_t kheap_ptr = kheap_start;
  biggest_order = 0;
  // NOTE: la boucle doit se faire à l'envers car un bloc de taille 2^k doit
  // toujours commencer à une adresse multiple de 2^k (tous les bits d'indice
  // inférieur à k sont à 0) pour que le XOR du buddy fonctionne.
  for (int order = MAX_ORDER - 1; order >= (int)MIN_ORDER; order--) {
    uint64_t bit = (kheap_size >> order) & 1;
    if (bit == 1) {
      if (order > biggest_order) {
        biggest_order = order;
      }
      heap_fl_t block = {
          .order = order, .is_free = 1, .prev = NULL, .next = NULL};
      *(heap_fl_t *)kheap_ptr = block;
      kheap_arr[order] = (heap_fl_t *)kheap_ptr;
      kheap_ptr += (1ULL << order);
    }
  }
}

// Ajoute le block au début de la free list d'ordre order
inline static void list_prepend(uint8_t order, heap_fl_t *block) {
  if (block == NULL) {
    return;
  }

  block->is_free = 1;
  block->order = order;
  block->prev = NULL;
  block->next = kheap_arr[order];
  if (kheap_arr[order] != NULL) {
    kheap_arr[order]->prev = block;
  }
  kheap_arr[order] = block;
}

// Extrait le block de la free list
inline static void list_remove(uint8_t order, heap_fl_t *block) {
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
    kheap_arr[order] = block->next;
  }
}

// Coupe un bloc libre en deux blocs d'ordre inférieur (utile pour malloc)
static void split_free_block(uint8_t order, heap_fl_t *block) {
  if (order == 0) {
    return; // On ne peut pas split un bloc d'ordre 0
  }

  // On supprime le bloc dans la free list originale
  list_remove(order, block);

  // Puis on ajoute deux blocs à la free list d'ordre inférieur
  uint8_t new_order = order - 1;
  uint64_t new_block_size = 1ULL << new_order;

  heap_fl_t *new_block1 = block;
  heap_fl_t *new_block2 = (heap_fl_t *)((uintptr_t)new_block1 + new_block_size);
  list_prepend(new_order, new_block2);
  list_prepend(new_order, new_block1);
}

// Alloue un bloc de mémoire de taille memorySize octets dans le tas.
void *kmalloc(size_t memorySize) {
  uint8_t order = integer_log2(memorySize + FL_REDUCED_HEADER_SIZE);

  if (order > biggest_order) {
    printf("kmalloc error: Allocation dynamique trop importante.\n");
    return NULL;
  }

  if (order < MIN_ORDER) {
    order = MIN_ORDER;
  }

  // On cherche un bloc de taille 2^order
  heap_fl_t *free_bloc = kheap_arr[order];
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
    while (k <= biggest_order && kheap_arr[k] == NULL) {
      k++;
    }

    if (k > biggest_order) {
      return NULL; // le tas est plein
    }

    // On splitte ce bloc
    split_free_block(k, kheap_arr[k]);
    k--; // l'ordre le plus proche de order a diminué de 1 car on a créé de
    // nouveaux blocs d'ordre inférieur
  }

  // On retourne le bloc (sans header) après l'avoir retiré de la free list
  heap_fl_t *block_found = kheap_arr[order];
  void *allocated_block =
      (void *)((uintptr_t)block_found + FL_REDUCED_HEADER_SIZE);
  list_remove(order, block_found);
  return allocated_block;
}

// Libère le pointeur alloué dynamiquement sur le tas
void kfree(void *ptr) {
  if (ptr == NULL) {
    printf("kfree error: Tentative de free NULL\n");
    return;
  }

  if ((uintptr_t)ptr < kheap_start || (uintptr_t)ptr > kheap_end) {
    printf("kfree error: Tentative de free un pointeur qui n'est pas sur "
           "le tas\n");
    return;
  }

  heap_fl_t *block = (heap_fl_t *)((uintptr_t)ptr - FL_REDUCED_HEADER_SIZE);
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
    uintptr_t buddy_addr = kheap_start + (((uintptr_t)block - kheap_start) ^
                                          (uintptr_t)(1ULL << order));
    heap_fl_t *buddy = (heap_fl_t *)buddy_addr;

    if (buddy_addr >= kheap_end || buddy->is_free == 0 ||
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
void *kcalloc(size_t elementCount, size_t elementSize) {
  size_t total_size = elementCount * elementSize;
  void *ptr = kmalloc(total_size);
  if (ptr == NULL) {
    return NULL;
  }

  memset(ptr, 0, total_size);
  return ptr;
}

// Réalloue le bloc de mémoire à l'adresse pointer, de taille memorySize dans le
// tas, en l'agrandissant ou le rétressissant. Retourne l'adresse du nouveau
// bloc alloué
void *krealloc(void *pointer, size_t memorySize) {
  if (pointer == NULL) {
    // C'est un malloc classique
    return kmalloc(memorySize);
  }

  if (memorySize == 0) {
    // C'est un free
    kfree(pointer);
    return NULL;
  }

  heap_fl_t *block = (heap_fl_t *)((uintptr_t)pointer - FL_REDUCED_HEADER_SIZE);
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
  void *new_ptr = kmalloc(memorySize);
  if (new_ptr == NULL) {
    printf("realloc error\n");
    return NULL;
  }

  // On copie les données de l'ancien vers le nouveau
  // Si le realloc veut tronquer les données alors on tronque
  size_t new_size = (memorySize < cur_size) ? memorySize : cur_size;
  memcpy(new_ptr, pointer, new_size);

  kfree(pointer);

  return new_ptr;
}
