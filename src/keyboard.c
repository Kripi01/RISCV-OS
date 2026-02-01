#include "keyboard.h"
#include "platform.h"
#include <stdint.h>

int getchar_uart() {
  // NOTE: La réception (et la config en général) de l'UART est activée dans
  // init_uart

  // Bien que le registre d'écriture et de lecture ont la même adresse, ce sont
  // deux registres différents. Quand on fait une lecture, le Write Enable de la
  // mémoire (WE barre en haut) est à 0, et quand on fait une écriture , WE est
  // à 1. L'UART dirige vers un registre différent selon la valeur de WE et
  // comme une mémoire est accessible en lecture OU EXCLUSIF en écriture à un
  // instant donné, il n'y a pas de problèmes.
  volatile uint8_t *addr = (uint8_t *)(UART_BASE + UART_RBR);
  char c = *addr;
  // On gère le problème du 'Entrée' interprété comme un CR et non comme un LF
  // (ou CRLF)
  if (c == '\r' || c == '\n') {
    return '\n';
  }

  return c;
}
