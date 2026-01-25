#include <cpu.h>
#include <stdint.h>
#include <stdio.h>

void init_uart() {
  // NOTE: UART_BAUD_RATE et UART_CLOCK_FREQ sont déjà correctement configurés
  // dans platform.h
  uint16_t div_freq = UART_CLOCK_FREQ / (16 * UART_BAUD_RATE);

  *(uint8_t *)(UART_BASE + UART_LCR) |= 0x80;

  *(uint8_t *)(UART_BASE + UART_DLL) = div_freq;      // Poids faible
  *(uint8_t *)(UART_BASE + UART_DLH) = div_freq >> 8; // Poids fort

  // On active les files d'attentes pour la réception et l'émission
  *(uint8_t *)(UART_BASE + UART_FCR) |= 0x1;

  // On sélectionne 8 bits pour la transmission
  *(uint8_t *)(UART_BASE + UART_LCR) |= 0x3;
  // On active le bit de parité
  *(uint8_t *)(UART_BASE + UART_LCR) |= 0x8;

  // On ne sélectionne plus le mode de configuration de la vitesse pour pouvoir
  // accèder au registre Transmitter Holding Register par la suite
  *(uint8_t *)(UART_BASE + UART_LCR) &= 0x7F;
}

void traite_car_uart(char c) {
  uint8_t *addr = (uint8_t *)(UART_BASE + UART_THR);
  *addr = c;
}

void console_put_bytes(const char *s, int len) {
  for (int i = 0; i < len; i++) {
    traite_car_uart(s[i]);
  }
}

void kernel_start() {
  init_uart();

  console_put_bytes("Hugo\n", 5);

  /* on ne doit jamais sortir de kernel_start */
  while (1) {
    /* cette fonction arrete le processeur */
    hlt();
  }
}
