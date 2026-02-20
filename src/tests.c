#include "tests.h"
#include "buddy_heap.h"
#include "syscalls.h"
#include <stdio.h>

/* ====================================== */
/* ==== BUDDY MEMORY ALLOCATION TEST ==== */
/* ====================================== */

// BUG: adresses relatives (auipc) donc l'adresse de buddy_malloc est avant
// buddy_heap_test, tellement avant qu'elle est avant 0x40000000
// FIX: on la force à être après dans le linker script (mais c'est vraiment du
// bricolage)
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
      UPRINTF("%d ", mat[i][j], 0, 0, 0, 0, 0);
    }
    UPUTS("\n");
  }

  // Puis on free la matrice
  for (int i = 0; i < N; i++) {
    buddy_free(mat[i]);
  }
  buddy_free(mat);

  UEXIT(0);
}

int buddy_heap_overflow_test() {
  int size = 128000000; // overflow! (la ram fait 128Mo)
  char *tab = buddy_malloc(size * sizeof(char));
  if (tab == NULL) {
    UEXIT(1);
  }

  for (int i = 0; i < size; i++) {
    if (i % 10000000 == 0) {
      UPRINTF("i: %d\n", i, 0, 0, 0, 0, 0);
    }
    tab[i] = 'b';
  }

  buddy_free(tab);

  UEXIT(0);
}
