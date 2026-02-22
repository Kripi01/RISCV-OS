#ifndef __PM_H__
#define __PM_H__

#include <stdint.h>

#define FRAMESIZE 4096 // 4 kilo bytes

// On représente les frames libres par une free list
typedef struct free_list_s {
  struct free_list_s *next;
} free_list_t;

void init_frames();
void *get_frame();
void release_frame(void *frame);

#endif // __PM_H__
