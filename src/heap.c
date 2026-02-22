#include "heap.h"

// Retourne la partie supérieure de log2(x) (= le plus haut bit à 1 de x)
uint8_t integer_log2(uint64_t x) {
  if (x <= 1) {
    return 0; // Par commodité, on pose log2(0) = 0
  }

  // On cherche le plus haut bit valant 1 dans x-1 (pour éviter le pb de la
  // puissance de deux exacte)
  x--;
  uint8_t i = 0;
  while (x != 0) {
    x >>= 1;
    i++;
  }

  return i;
}
