#include "keyboard.h"
#include "platform.h"
#include "screen.h"
#include "shell.h"
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
  *(volatile uint8_t *)(UART_BASE + UART_IER) |= 0x1;

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
  char c = *(volatile uint8_t *)(UART_BASE + UART_RBR);

  // Le buffer est vide, donc c'est une nouvelle commande que l'on tape. Donc on
  // ne peut plus revenir en arrière (fix le bug de suppression des résultats
  // des commandes précédentes e.g. ps)
  if (last_index_cmd == 0) {
    set_validated_lig();
  }

  // On gère le problème du 'Entrée' interprété comme un CR et non comme un LF
  // (ou CRLF)
  if (c == '\r' || c == '\n') {
    command_str[last_index_cmd] = '\n';
    return '\n';
  }

  // Quand c'est un SUPPR, on doit supprimer un caractère de la commande.
  if (c == 127 || c == '\b') {
    if (last_index_cmd > 0) {
      last_index_cmd--;
      command_str[last_index_cmd] = 0;
    }
  } else if (last_index_cmd < MAX_LENGTH_COMMANDS) {
    // On ajoute le caractère au tableau de bash.
    command_str[last_index_cmd] = c;
    last_index_cmd++;
  }

  return c;
}
