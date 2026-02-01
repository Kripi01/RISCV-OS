// TODO: volatile

#include "keyboard.h"
#include "platform.h"
#include <stdint.h>

/* Configure le PLIC pour autoriser IRQ10 (UART)*/
void configPLIC_for_UART() {
  *(volatile uint32_t *)PLIC_ENABLE = 1 << 10;
  /* La priorité de l'UART est fixée à 3 */
  *(volatile uint32_t *)(PLIC_SOURCE + (10 << 2)) = 3;
  /* On règle le seuil de déclenchement des interruptions à 0 (toutes priorité
   * d'interruption) */
  *(volatile uint32_t *)PLIC_TARGET = 0;
}

void enable_uart_interrupts() {
  // On active les interruptions à la réception (RX) d'un caractère
  *(uint8_t *)(UART_BASE + UART_IER) |= 0x1;

  // On autorise les interruptions externes
  __asm__("csrs mie, %0" ::"r"(1 << IRQ_M_EXT));

  configPLIC_for_UART();
}

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
