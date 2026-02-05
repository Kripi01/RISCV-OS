// Allocation de mémoire dynamique:
// IDÉE (naïve): on stocke pour chaque bloc de données alloué dans le tas, un
// header contenant toutes les infos nécessaires pour pouvoir free et réallouer
// par la suite.
// PERF: malloc en O(n) car on doit parcourir tout le tas pour trouver un
// emplacement convenable.

// TODO: Implémenter un segregated fits (au lieu de la version naïve, first
// fits).

#include "heap.h"
#include <stdint.h>

// _heap_start commence  après la fin de tout le code du kernel (cf kernel.lds)
// NOTE: _heap_start est juste un symbole (=étiquette) défini par le linker, ce
// n'est pas une variable. On ne déréférence jamais ce symbole, on utilise
// uniquement son adresse, l'adresse du début du tas. Il faut tout de même
// choisir un type, on prend char car c'est le plus neutre possible, c'est juste
// un octet.
extern char _heap_start;

// adresse du bloc le plus haut du tas
uintptr_t heap_ptr = (uintptr_t)&_heap_start;

// L'adresse que l'on alloue est l'adresse juste après le header d'infos.
// malloc en O(n) car on parcourt tous les blocs, dans le pire cas, pour trouver
// un bloc libre de taille suffisante.
// WARNING: memorySize est la taille en octets du bloc à allouer.
// WARNING: Pas de gestion d'overflow!
void *h_malloc(size_t memorySize) {
  // On parcourt le tas jusqu'à trouver un bloc libre
  uintptr_t p = (uintptr_t)&_heap_start;

  while (p < heap_ptr) {
    heap_header_t *p_header = (heap_header_t *)p;
    if (p_header->is_free && p_header->block_size >= memorySize) {
      // On ne change pas block_size (on perd un peu de mémoire car c'est un
      // first fit et pas un best fit)
      p_header->is_free = 0;
      p += HEADER_SIZE;

      return (void *)p;
    }

    p += HEADER_SIZE;
    p += p_header->block_size;
  }

  // Si on arrive ici, c'est qu'on a pas trouvé de bloc libre, donc on en alloue
  // un nouveau.
  heap_header_t data = {.block_size = memorySize, .is_free = 0};
  *(heap_header_t *)heap_ptr = data;
  heap_ptr += HEADER_SIZE;

  void *allocated_ptr = (void *)heap_ptr;
  heap_ptr += memorySize;

  return allocated_ptr;
}

// Pour free, il suffit d'indiquer que le bloc est libre. Pour cela, on change
// le header du bloc à l'adresse: adresse fournie - sizeof(heap_header_t)
void h_free(void *ptr) {
  ((heap_header_t *)ptr - 1)->is_free = 1;
  return;
}
