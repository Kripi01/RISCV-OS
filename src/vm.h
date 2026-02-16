// On utilise le standard Sv39 de RISCV 64 bits, on peut donc adresser au
// maximum 512GB de mémoire
//
// Structure d'une va (Sv39):
// 63 -- 39 38 ----- 30 29 ----- 21 20 ----- 12 11 -------- 0
// Unused || L2 Index || L1 Index || L0 Index || Page Offset
//
// Structure d'une pa (Sv39):
// 63 -- 56 55 ----- 30 29 ----- 21 20 ----- 12 11 -------- 0
// Unused ||  PPN[2]  ||  PPN[1]  ||  PPN[0]  || Page Offset

#ifndef __VM_H__
#define __VM_H__

#include <stdint.h>

#define PAGESIZE 4096 // 4 kilo bytes
#define PTESIZE 8
#define PTE_PER_PAGE (PAGESIZE / PTESIZE) // 512 PTE par page
#define LEVELS 3

// Page Table Entry
// Chaque PTE mappe un VPN vers un PPN + flags:
// Structure d'une pte (Sv39):
// 63 --- 61 60 ----- 54 53 ----- 28 27 ----- 19 18 ----- 10 9 -- 8 7 ------ 0
//   0000  || Reserved ||  PPN[2]  ||  PPN[1]  ||  PPN[0]  || RSW || DAGUXWRV
typedef uint64_t pte_t;

// pagetable_t est un tableau de PTE_PER_PAGE pte_t indicé par les VPN
typedef pte_t *pagetable_t;

// Conversions de va vers vpn:
#define GET_LINDEX(va, level) (((va) >> (12 + 9 * level)) & 0x1FF)
#define GET_OFFSET(va) ((va) & 0xFFF)

// Utilitaires des PTE
#define GET_FLAGS(pte) ((pte) & 0xFF)
#define PTE2PA(pte) (((uintptr_t)(pte) >> 10) << 12)
#define PA2PTE(pa) (((uintptr_t)(pa) >> 12) << 10)
#define GET_PPN(pte) ((pte >> 10) & 0x7FFFFFFFFFF)

uint64_t init_vm(uint64_t asid);
void raise_page_fault();
pte_t *walk(pagetable_t pt, uintptr_t va, int alloc);
void map_page(pagetable_t base, uintptr_t va, uintptr_t pa, uint64_t flags);

#endif // __VM_H__
