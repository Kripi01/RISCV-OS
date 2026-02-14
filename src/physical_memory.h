#ifndef __FRAME_H__
#define __FRAME_H__

#include <stdint.h>

#define FRAMESIZE 4096 // 4 kilo bytes

// On représente les frames libres par une free list (cf. buddy_heap.h)
typedef struct free_list_s {
  struct free_list_s *next;
} free_list_t;

void init_frames();
void *get_frame();
void release_frame(void *frame);

#endif // __FRAME_H__
