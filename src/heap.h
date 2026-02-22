#ifndef __HEAP_H__
#define __HEAP_H__

#include <stddef.h>
#include <stdint.h>

typedef struct heap_fl_s {
  uint8_t order;
  uint8_t is_free;
  struct heap_fl_s *prev;
  struct heap_fl_s *next;
} heap_fl_t;

#define FL_HEADER_SIZE sizeof(heap_fl_t)
// On gagne 16 octets en laissant l'utilisateur écraser prev et next
#define FL_REDUCED_HEADER_SIZE 8

// Il faut que l'ordre minimal soit strictement supérieur à la taille en
// bytes du header (=32) parce que sinon les blocs overlappent
#define MIN_ORDER 5
#define MAX_ORDER 64

uint8_t integer_log2(uint64_t x);

#endif // __HEAP_H__
