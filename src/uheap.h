#ifndef __UHEAP_H__
#define __UHEAP_H__

#include "heap.h"
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
#define HEAP_ARR(order) (((heap_fl_t **)(HEAP_ARR_START))[order])

void *umalloc(size_t memorySize);
void ufree(void *ptr);
void *ucalloc(size_t elementCount, size_t elementSize);
void *urealloc(void *pointer, size_t memorySize);

#endif // __UHEAP_H__
