#ifndef __PLATFORM_H__
#define __PLATFORM_H__

#if __ASSEMBLER__ == 0
#include <stdint.h>
#endif

// Video memory base address
#define BOCHS_DISPLAY_BASE_ADDRESS 0x50000000
#define BOCHS_CONFIG_BASE_ADDRESS 0x40000000

/* Adresse des différents registres de config */
#define VBE_DISPI_INDEX_ID 0x00
#define VBE_DISPI_INDEX_XRES 0x01
#define VBE_DISPI_INDEX_YRES 0x02
#define VBE_DISPI_INDEX_BPP 0x03
#define VBE_DISPI_INDEX_ENABLE 0x04
#define VBE_DISPI_INDEX_BANK 0x05
#define VBE_DISPI_INDEX_VIRT_WIDTH 0x06
#define VBE_DISPI_INDEX_VIRT_HEIGHT 0x07
#define VBE_DISPI_INDEX_X_OFFSET 0x08
#define VBE_DISPI_INDEX_Y_OFFSET 0x09
#define VBE_DISPI_INDEX_VIDEO_MEMORY_64K 0x0a
#define VBE_DISPI_INDEX_ENDIAN 0x82

#define VBE_DISPI_ID0 0xb0c0
#define VBE_DISPI_ID1 0xb0c1
#define VBE_DISPI_ID2 0xb0c2
#define VBE_DISPI_ID3 0xb0c3
#define VBE_DISPI_ID4 0xb0c4
#define VBE_DISPI_ID5 0xb0c5

#define VBE_DISPI_DISABLED 0x00
#define VBE_DISPI_ENABLED 0x01
#define VBE_DISPI_GETCAPS 0x02
#define VBE_DISPI_8BIT_DAC 0x20
#define VBE_DISPI_LFB_ENABLED 0x40
#define VBE_DISPI_NOCLEARMEM 0x80

#define VBE_MAX_WIDTH 1024
#define VBE_MAX_HEIGHT 768

#define DISPLAY_ENDIAN 0x1e1e1e1e
#define DISPLAY_WIDTH 1024
#define DISPLAY_HEIGHT 768
#define DISPLAY_BPP 32
#define DISPLAY_SIZE (DISPLAY_WIDTH * DISPLAY_HEIGHT)

// Le nombre de bytes que l'écran occupe en mémoire
#define DISPLAY_MEMORY_SIZE (DISPLAY_SIZE * 4)

// Info bus PCI
#define PCI_ECAM_BASE_ADDRESS 0x30000000
#define DISPLAY_PCI_ID 0x11111234

// PLIC registers addresses
// Les registres pending bits ne sont normalement jamais utilisés car le PLIC
// les update automatiquement lors d'un claim
#define PLIC_PENDING_BASE 0x0c001000
#define PLIC_SOURCE_BASE 0x0c000000
#define PLIC_SOURCE(id) (PLIC_SOURCE_BASE + ((id) << 2))

// Adresses spécifiques au contexte
#define PLIC_ENABLE_BASE 0x0c002000
#define PLIC_ENABLE(id, ctxt)                                                  \
  (PLIC_ENABLE_BASE + (((id) % 8) >> 2) + ((ctxt) << 7))

#define PLIC_THRESHOLD_BASE 0x0c200000
#define PLIC_THRESHOLD(ctxt) (PLIC_THRESHOLD_BASE + ((ctxt) << 12))

#define PLIC_IRQ_CLAIM_BASE 0x0c200004
#define PLIC_IRQ_CLAIM(ctxt) (PLIC_IRQ_CLAIM_BASE + ((ctxt) << 12))

#define PLIC_MEMORY_SIZE 0x4000000

// PLIC pushbutton irq
#define PLIC_IRQ_2 0x2

// CLINT registers addresses
#define CLINT_MSIP 0x02000000
#define CLINT_TIMER_CMP 0x02004000
#define CLINT_TIMER_CMP_LO 0x02004000
#define CLINT_TIMER_CMP_HI 0x02004004
#define CLINT_TIMER 0x0200bff8
#define CLINT_TIMER_LOW 0x0200bff8
#define CLINT_TIMER_HI 0x0200bffc

// Timer options
#define TIMER_FREQ 10000000 // 10MHz
#define TIMER_RATIO 500

// Page Table Entry flags
#define PTE_V (1 << 0) // valid
#define PTE_R (1 << 1) // read permission
#define PTE_W (1 << 2) // write permission
#define PTE_X (1 << 3) // execute permission
#define PTE_U (1 << 4) // user mode permission
#define PTE_G (1 << 5) // global mapping
#define PTE_A (1 << 6) // accessed
#define PTE_D (1 << 7) // dirty
#define PTE_RWXV (PTE_R | PTE_W | PTE_X | PTE_V)
#define PTE_RWV (PTE_R | PTE_W | PTE_V)

// Bit in mstatus
#define MSTATUS_MIE 0x00000008
// Bit in sstatus
#define SSTATUS_SIE 0x00000002

// Bits in masked_mcause
#define IRQ_M_SFT 3
#define IRQ_M_TMR 7
#define IRQ_M_EXT 11
#define EXC_M_ENV_CALL_FROM_U 8
#define EXC_M_ENV_CALL_FROM_S 9
#define EXC_M_ENV_CALL_FROM_M 11

// Bits in masked_sscause
#define IRQ_S_SFT 1
#define IRQ_S_TMR 5
#define IRQ_S_EXT 9
// Page faults
#define EXC_INSTRUCTION_PF 12
#define EXC_LOAD_PF 13
#define EXC_STORE_PF 15
#define EXC_IS_PF(scause)                                                      \
  ((scause) == EXC_INSTRUCTION_PF || (scause) == EXC_LOAD_PF ||                \
   (scause) == EXC_STORE_PF)

// Bits in mie/sie
#define SSIE (1 << 1)
#define MSIE (1 << 3)
#define STIE (1 << 5)
#define MTIE (1 << 7)
#define SEIE (1 << 9)
#define MEIE (1 << 11)

// Bits in mip/sip
#define SSIP (1 << 1)
#define MSIP (1 << 3)
#define STIP (1 << 5)
#define MTIP (1 << 7)
#define SEIP (1 << 9)
#define MEIP (1 << 11)

// UART
#define UART_BASE 0x10000000
#define UART_CLOCK_FREQ 1843200
#define UART_BAUD_RATE 115200

#define UART_IER_MSI 0x08  /* Enable Modem status interrupt */
#define UART_IER_RLSI 0x04 /* Enable receiver line status interrupt */
#define UART_IER_THRI 0x02 /* Enable Transmitter holding register int. */
#define UART_IER_RDI 0x01  /* Enable receiver data interrupt */

#define UART_IIR_NO_INT 0x01 /* No interrupts pending */
#define UART_IIR_ID 0x06     /* Mask for the interrupt ID */

enum {
  UART_RBR = 0x00, /* Receive Buffer Register */
  UART_THR = 0x00, /* Transmit Hold Register */
  UART_IER = 0x01, /* Interrupt Enable Register */
  UART_DLL = 0x00, /* Divisor LSB (LCR_DLAB) */
  UART_DLH = 0x01, /* Divisor MSB (LCR_DLAB) */
  UART_FCR = 0x02, /* FIFO Control Register */
  UART_LCR = 0x03, /* Line Control Register */
  UART_MCR = 0x04, /* Modem Control Register */
  UART_LSR = 0x05, /* Line Status Register */
  UART_MSR = 0x06, /* Modem Status Register */
  UART_SCR = 0x07, /* Scratch Register */

  UART_LCR_DLAB = 0x80, /* Divisor Latch Bit */
  UART_LCR_8BIT = 0x03, /* 8-bit */
  UART_LCR_PODD = 0x08, /* Parity Odd */

  UART_LSR_DA = 0x01, /* Data Available */
  UART_LSR_OE = 0x02, /* Overrun Error */
  UART_LSR_PE = 0x04, /* Parity Error */
  UART_LSR_FE = 0x08, /* Framing Error */
  UART_LSR_BI = 0x10, /* Break indicator */
  UART_LSR_RE = 0x20, /* THR is empty */
  UART_LSR_RI = 0x40, /* THR is empty and line is idle */
  UART_LSR_EF = 0x80, /* Erroneous data in FIFO */
};

enum {
  /* UART Registers */
  UART_REG_TXFIFO = 0,
  UART_REG_RXFIFO = 1,
  UART_REG_TXCTRL = 2,
  UART_REG_RXCTRL = 3,
  UART_REG_IE = 4,
  UART_REG_IP = 5,
  UART_REG_DIV = 6,

  /* TXCTRL register */
  UART_TXEN = 1,
  UART_TXSTOP = 2,

  /* RXCTRL register */
  UART_RXEN = 1,

  /* IP register */
  UART_IP_TXWM = 1,
  UART_IP_RXWM = 2,

  /* INTERRUPT ENABLE */
  UART_RX_IT_EN = 2,
  UART_TX_IT_EN = 1
};

#if __ASSEMBLER__ == 0
extern void timer_set(uint32_t period, uint32_t start_value);
extern void timer_wait();
extern void timer_set_and_wait(uint32_t period, uint32_t time);
extern void led_set(uint32_t value);
extern uint32_t push_button_get();
#endif

#endif // __PLATFORM_H__
