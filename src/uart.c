#include "platform.h"
#include <stdint.h>

// NOTE: Tous les registres de l'UART sont sur 8 bits

void init_uart() {
  // NOTE: UART_BAUD_RATE et UART_CLOCK_FREQ sont déjà correctement configurés
  // dans platform.h
  uint16_t div_freq = UART_CLOCK_FREQ / (16 * UART_BAUD_RATE);

  *(volatile uint8_t *)(UART_BASE + UART_LCR) |= 0x80;

  *(volatile uint8_t *)(UART_BASE + UART_DLL) = div_freq;      // Poids faible
  *(volatile uint8_t *)(UART_BASE + UART_DLH) = div_freq >> 8; // Poids fort

  // On active les files d'attentes pour la réception et l'émission
  *(volatile uint8_t *)(UART_BASE + UART_FCR) |= 0x1;

  // On sélectionne 8 bits pour la transmission
  *(volatile uint8_t *)(UART_BASE + UART_LCR) |= 0x3;
  // On active le bit de parité
  *(volatile uint8_t *)(UART_BASE + UART_LCR) |= 0x8;

  // On ne sélectionne plus le mode de configuration de la vitesse pour pouvoir
  // accèder au registre Transmitter Holding Register par la suite
  *(volatile uint8_t *)(UART_BASE + UART_LCR) &= 0x7F;
}

void traite_car_uart(char c) {
  *(volatile uint8_t *)(UART_BASE + UART_THR) = c;
}
