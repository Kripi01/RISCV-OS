// Allocation de mémoire dynamique:
// IDÉE (naïve): on stocke pour chaque bloc de données alloué dans le tas, un
// header contenant toutes les infos nécessaires pour pouvoir free et réallouer
// par la suite.
// PERF: malloc en O(n) car on doit parcourir tout le tas pour trouver un
// emplacement convenable.
// WARNING: Il faut utiliser l'implémentation du Buddy Memory Allocation dans
// buddy_heap.c

#include "naive_heap.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>

// _heap_start commence  après la fin de tout le code du kernel (cf kernel.lds)
// NOTE: _heap_start est juste un symbole (=étiquette) défini par le linker, ce
// n'est pas une variable. On ne déréférence jamais ce symbole, on utilise
// uniquement son adresse, l'adresse du début du tas. Il faut tout de même
// choisir un type, on prend char car c'est le plus neutre possible, c'est juste
// un octet.
extern char _heap_start;

// adresse du bloc le plus haut du tas
uintptr_t heap_ptr = (uintptr_t)&_heap_start;

// Alloue un bloc de mémoire de taille memorySize octets dans le tas. Pas de
// gestion d'overflow L'adresse que l'on renvoie est l'adresse juste après le
// header d'infos.
// PERF: malloc en O(n) car on parcourt tous les blocs, dans le
// pire cas, pour trouver un bloc libre de taille suffisante.
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

// Alloue un bloc de mémoire dans le tas, de taille elementCount éléments de
// taille elementSize, et initialisé à 0.
void *h_calloc(size_t elementCount, size_t elementSize) {
  size_t blocSize = elementCount * elementSize;
  void *allocated_ptr = h_malloc(blocSize);
  // On remplit le bloc alloué de 0.
  memset(allocated_ptr, 0, blocSize);

  return allocated_ptr;
}

// Réalloue un bloc de mémoire dans le tas.
// Si l'espace mémoire libre qui suit le bloc à réallouer est suffisament grand,
// le bloc de mémoire est simplement agrandi. Sinon, un nouveau bloc de mémoire
// est alloué, le contenu de la zone d'origine recopié dans la nouvelle zone et
// le bloc mémoire d'origine sera libéré automatiquement.
void *h_realloc(void *pointer, size_t memorySize) {
  heap_header_t *header = (heap_header_t *)((uintptr_t)pointer - HEADER_SIZE);
  size_t cur_blocSize = header->block_size;
  if (memorySize <= cur_blocSize) { // Pas besoin d'agrandir
    return pointer;
  }

  heap_header_t *next_header =
      (heap_header_t *)((uintptr_t)pointer + cur_blocSize);
  if ((uintptr_t)next_header < heap_ptr && next_header->is_free == 1 &&
      next_header->block_size + cur_blocSize >= memorySize) {
    // On agrandit en ajoutant juste le bloc suivant (avec son header!).
    header->block_size += next_header->block_size + HEADER_SIZE;
    // On ne touche pas au next_header, il sera écrasé par les données de
    // l'utilisateur.
    return pointer;
  }

  // Le bloc suivant n'est pas assez grand, donc on alloue un nouveau bloc de
  // mémoire, et on recopie le contenu.
  void *new_bloc = h_malloc(memorySize);
  // memcpy pour la rapidité, on peut car il n'y a pas de chevauchement
  memcpy(new_bloc, pointer, header->block_size);
  header->is_free = 1; // On libère le bloc original

  return new_bloc;
}

// Pour free, il suffit d'indiquer que le bloc est libre. Pour cela, on change
// le header du bloc à l'adresse: adresse fournie - sizeof(heap_header_t)
void h_free(void *ptr) {
  ((heap_header_t *)ptr - 1)->is_free = 1;
  return;
}
