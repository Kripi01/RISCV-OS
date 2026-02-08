#include "tests.h"
#include "buddy_heap.h"
#include <stdio.h>

/* ====================================== */
/* ==== BUDDY MEMORY ALLOCATION TEST ==== */
/* ====================================== */

int buddy_heap_test() {
  int N = 10;

  int **mat = buddy_malloc(N * sizeof(int *));
  for (int i = 0; i < N; i++) {
    mat[i] = buddy_malloc(N * sizeof(int));
    for (int j = 0; j < N; j++) {
      mat[i][j] = i * N + j;
    }
  }

  for (int i = 0; i < N; i++) {
    for (int j = 0; j < N; j++) {
      printf("%d ", mat[i][j]);
    }
    printf("\n");
  }

  // Puis on free la matrice
  for (int i = 0; i < N; i++) {
    buddy_free(mat[i]);
  }
  buddy_free(mat);

  return 0;
}

int buddy_heap_overflow_test() {
  int size = 128000000; // overflow! (la ram fait 128Mo)
  char *tab = buddy_malloc(size * sizeof(char));
  if (tab == NULL) {
    return 1;
  }

  for (int i = 0; i < size; i++) {
    if (i % 10000000 == 0) {
      printf("i: %d\n", i);
    }
    tab[i] = 'b';
  }

  buddy_free(tab);

  return 0;
}
