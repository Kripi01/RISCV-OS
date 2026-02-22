#ifndef __KHEAP_H__
#define __KHEAP_H__

#include "heap.h"

void init_kheap();
void *kmalloc(size_t memorySize);
void kfree(void *ptr);
void *kcalloc(size_t elementCount, size_t elementSize);
void *krealloc(void *pointer, size_t memorySize);

#endif // __KHEAP_H__
