// TODO: Utiliser le buddy_allocator sur des blocs de taille minimale FRAMESIZE
// pour vraiment copier le kernel Linux

// SOURCES:
// https://wiki.osdev.org/Memory_Allocation
// https://www.kernel.org/doc/gorman/html/understand/understand009.html

#include "physical_memory.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

extern char _free_memory_start;
// free_memory_start est déjà aligné sur 4KB (cf. kernel.lds)
const uintptr_t free_memory_start = (uintptr_t)&_free_memory_start;

extern char _memory_end;
const uintptr_t memory_end = (uintptr_t)&_memory_end;

free_list_t *fl_head;
size_t bitmap_size;
uint8_t *bitmap; // 0 pour une frame libre et 1 pour une frame utilisé

#define GET_PFN(pa) (((uintptr_t)(pa) - free_memory_start) / FRAMESIZE)
#define IS_USED(pfn) ((bitmap[pfn / 8] >> (pfn % 8)) & 0x1)

#define CLEAR_BITMAP(pa)                                                       \
  {                                                                            \
    uint64_t pfn = GET_PFN(pa);                                                \
    bitmap[pfn / 8] &= ~(1 << (pfn % 8));                                      \
  }

#define SET_BITMAP(pa)                                                         \
  {                                                                            \
    uint64_t pfn = GET_PFN(pa);                                                \
    bitmap[pfn / 8] |= 1 << (pfn % 8);                                         \
  }

#define ALIGN_UP(x) (((x) + FRAMESIZE - 1) & ~(FRAMESIZE - 1))

// Initialise la mémoire physique libre en remplissant toute la free list
void init_frames() {
  // On initialise la bitmap
  uintptr_t nb_frames = (memory_end - free_memory_start) / FRAMESIZE;
  bitmap_size = (nb_frames + 7) / 8; // 1 bit par frame, arrondi supérieur
  bitmap = (uint8_t *)free_memory_start;
  memset(bitmap, 0xFF, bitmap_size); // on remplit chaque BYTE

  // on aligne fl_head sur 4096 bytes
  fl_head = (free_list_t *)ALIGN_UP(free_memory_start + bitmap_size);

  if ((uintptr_t)fl_head > memory_end) {
    printf("init_frames error: mémoire insuffisante\n");
    return;
  }

  fl_head->next = NULL;
  // NOTE: la première frame n'est pas fl_head (la première est au moins bitmap)
  CLEAR_BITMAP(fl_head);

  free_list_t *cur = fl_head;
  for (uintptr_t pa = (uintptr_t)fl_head + FRAMESIZE; pa < memory_end;
       pa += FRAMESIZE) {
    // on ajoute la frame à la free list. Les headers sont espacés de 4KB
    cur->next = (free_list_t *)pa;
    cur = (free_list_t *)pa;

    CLEAR_BITMAP(pa);
  }
  cur->next = NULL; // la dernière frame doit pointer vers NULL
}

// Retourne une frame libre
void *get_frame() {
  if (fl_head == NULL) {
    printf("get_frame error: plus de frame disponible\n");
    return NULL;
  }

  // On renvoie la première frame de la free list
  free_list_t *res = fl_head;
  fl_head = fl_head->next;
  SET_BITMAP(res);

  return res;
}

// Libère une frame.
void release_frame(void *frame) {
  if (frame == NULL) {
    return;
  }

  if ((((uintptr_t)frame) & 0xFFF) != 0) {
    printf("release_frame error: adresse non alignée sur 4096 bytes\n");
    return;
  }

  uintptr_t endframe_addr = (uintptr_t)frame + FRAMESIZE;
  if ((uintptr_t)frame < free_memory_start || endframe_addr > memory_end) {
    printf("release_frame error: adresse hors de la RAM\n");
    return;
  }

  if (IS_USED(GET_PFN(frame)) == 0) {
    printf("release_frame error: tentative de double libération\n");
    return;
  }

  // on ajoute la frame à libérer au début de la free list
  free_list_t *fl_frame = (free_list_t *)frame;
  fl_frame->next = fl_head;
  fl_head = fl_frame;
  CLEAR_BITMAP(fl_head);
}
