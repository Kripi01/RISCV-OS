#ifndef __KEYBOARD_H__
#define __KEYBOARD_H__

int getchar_uart();
void enable_uart_interrupts();
void configPLIC_for_UART();

#endif // __KEYBOARD_H__
