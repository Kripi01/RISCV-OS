#ifndef __BUDDY_HEAP_H__
#define __BUDDY_HEAP_H__

#include "vm.h"
#include <stddef.h>
#include <stdint.h>

#define HEAP_START 0x45000000UL
// Chaque processus a 10 pages allouées pour le tas
#define HEAP_SIZE (uint64_t)(10 * PAGESIZE)
// TODO: faire un brk pour augmenter la taille du tas du processus
#define HEAP_END (uint64_t)(HEAP_START + HEAP_SIZE)

// On stocke le heap_arr sur la page avant HEAP_START (heap_arr fait 512 bytes)
#define HEAP_ARR_START (HEAP_START - PAGESIZE)
#define HEAP_ARR(order) (((buddy_free_list_t **)(HEAP_ARR_START))[order])

typedef struct buddy_free_list_s {
  uint8_t order;
  uint8_t is_free;
  struct buddy_free_list_s *prev;
  struct buddy_free_list_s *next;
} buddy_free_list_t;

#define FL_HEADER_SIZE sizeof(free_list_t)
// On gagne 16 octets en laissant l'utilisateur écraser prev et next
#define FL_REDUCED_HEADER_SIZE 8
// Il faut que l'ordre minimal soit strictement supérieur à la taille en
// bytes du header (=32) parce que sinon les blocs overlappent
#define MIN_ORDER 5
#define MAX_ORDER 64

void *buddy_malloc(size_t memorySize);
void buddy_free(void *ptr);
void *buddy_calloc(size_t elementCount, size_t elementSize);
void *buddy_realloc(void *pointer, size_t memorySize);

#endif // __BUDDY_HEAP_H__
