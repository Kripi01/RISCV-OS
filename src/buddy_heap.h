#ifndef __BUDDY_HEAP_H__
#define __BUDDY_HEAP_H__

#include <stddef.h>
#include <stdint.h>

typedef struct free_list_s {
  uint8_t order;
  uint8_t is_free;
  struct free_list_s *prev;
  struct free_list_s *next;
} free_list_t;

#define FL_HEADER_SIZE sizeof(free_list_t)
// On gagne 16 octets en laissant l'utilisateur écraser prev et next
#define FL_REDUCED_HEADER_SIZE 8
#define MAX_ORDER 64

void buddy_init_heap();
void *buddy_malloc(size_t memorySize);
void buddy_free(void *ptr);
void *buddy_calloc(size_t elementCount, size_t elementSize);
void *buddy_realloc(void *pointer, size_t memorySize);

#endif // __BUDDY_HEAP_H__
