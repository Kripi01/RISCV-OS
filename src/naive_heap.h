#ifndef __NAIVE_HEAP_H__
#define __NAIVE_HEAP_H__

#include <stddef.h>
#include <stdint.h>

typedef struct {
  size_t block_size;
  uint8_t is_free;
  // NOTE: On peut encore ajouter 7 octets au cas où (car le compilo fait un
  // padding pour atteindre 16 bytes, multiple de 64 bits)
} heap_header_t;

#define HEADER_SIZE sizeof(heap_header_t)

void *h_malloc(size_t memorySize);
void *h_calloc(size_t elementCount, size_t elementSize);
void *h_realloc(void *pointer, size_t memorySize);
void h_free(void *ptr);

#endif // __NAIVE_HEAP_H__
