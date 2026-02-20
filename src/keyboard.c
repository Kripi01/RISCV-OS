// DOCS:
// PLIC:
// https://courses.grainger.illinois.edu/ECE391/fa2025/docs/riscv-plic-1.0.0.pdf

// QEMU:
// https://www.qemu.org/docs/master/system/riscv/virt.html
// "Guest software should discover the devices that are present in the generated
// DTB." -> on génère un device tree blob (DTB) avec les commandes:
/* qemu-system-riscv64 -nographic -machine virt -m 128M -bios none -kernel
 * kernel.bin -machine */
// dumpdtb=riscv_virt.dtb dtc -I dtb -O dts riscv_virt.dtb -o riscv_virt.dts

#include "keyboard.h"
#include "platform.h"
#include "screen.h"
#include "shell.h"
#include <stdint.h>

// Configure le PLIC pour l'UART (IRQ10)
// Contexte 0 = mode M (0x0b = 11) sur qemu -machine virt
// Contexte 1 = mode S (0x09) sur qemu -machine virt
// On setup donc le contexte 1 pour l'UART
void configPLIC_for_UART() {
  // On autorise l'IRQ10 (UART) pour le contexte 1 (mode S)
  *(volatile uint32_t *)PLIC_ENABLE(10, 1) = 1 << 10;

  // La priorité de l'UART est fixée à 3
  *(volatile uint32_t *)PLIC_SOURCE(10) = 3;

  // On règle le seuil de déclenchement des interruptions à 0 (toutes priorité
  // d'interruption) pour le contexte 1
  *(volatile uint32_t *)PLIC_THRESHOLD(1) = 0;
}

void enable_uart_interrupts() {
  // On active les interruptions à la réception (RX) d'un caractère
  *(volatile uint8_t *)(UART_BASE + UART_IER) |= 0x1;

  // On autorise les interruptions externes
  __asm__("csrs sie, %0" ::"r"(1 << IRQ_S_EXT));

  configPLIC_for_UART();
}

int getchar_uart() {
  // NOTE: La réception (et la config en général) de l'UART est activée dans
  // init_uart

  // Bien que le registre d'écriture et de lecture aient la même adresse, ce
  // sont deux registres différents. Quand on fait une lecture, le Write Enable
  // de la mémoire (WE barre en haut) est à 0, et quand on fait une écriture,
  // WE est à 1. L'UART dirige vers un registre différent selon la valeur de WE
  // et comme une mémoire est accessible en lecture OU EXCLUSIF en écriture à un
  // instant donné, il n'y a pas de problèmes.
  char c = *(volatile uint8_t *)(UART_BASE + UART_RBR);

  // Le buffer est vide, donc c'est une nouvelle commande que l'on tape. Donc on
  // ne peut plus revenir en arrière (fix le bug de suppression des résultats
  // des commandes précédentes e.g. ps)
  if (last_index_cmd == 0) {
    set_validated();
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
