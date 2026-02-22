#ifndef __RAMFS_H__
#define __RAMFS_H__

#include <stddef.h>
#include <stdint.h>

#define FILENAME_MAXSIZE 4096

typedef struct file_s {
  char *name;
  int nb_children;
  struct file_s **children; // la liste des enfants est allouée sur le tas
} file_t;

void init_ramfs();
file_t *mkdir(char *name);
void ls();
void pwd();

#endif // __RAMFS_H__
