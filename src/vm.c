// DOCS:
// https://media.eyolfson.com/courses/utoronto/ece353/2024-winter/lecture-11.pdf
// https://cs326-s25.cs.usfca.edu/guides/page-tables

// TODO: Refaire une nouvelle page table pour chaque processus (satp)

#include "vm.h"
#include "platform.h"
#include "pm.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>

extern char _kernel_start;
uintptr_t addr_kernel_start = (uintptr_t)&_kernel_start;
extern uintptr_t memory_end;

// TODO: faire une fonction d'initialisation de satp (avec les modes, etc)

static void identity_map(pagetable_t root_pt_pa) {
  // On mappe l'adresse 0 vers une page n'ayant pas le flag valid, pour que le
  // déréférencement de NULL provoque une page fault
  map_page(root_pt_pa, 0, 0, 0);

  // On mappe la RAM
  for (uintptr_t addr = addr_kernel_start; addr < memory_end;
       addr += PAGESIZE) {
    map_page(root_pt_pa, addr, addr, PTE_R | PTE_X | PTE_W | PTE_V);
  }

  // On mappe les adresses de l'écran (config + pixels)
  // TODO: Utiliser des constantes de kernel.lds au lieu de valeurs hardcodés???
  for (uintptr_t offset = 0; offset < 0x1000; offset++) {
    map_page(root_pt_pa, BOCHS_DISPLAY_BASE_ADDRESS + (offset << 12),
             BOCHS_DISPLAY_BASE_ADDRESS + (offset << 12),
             PTE_R | PTE_W | PTE_V);
  }
  map_page(root_pt_pa, BOCHS_CONFIG_BASE_ADDRESS, BOCHS_CONFIG_BASE_ADDRESS,
           PTE_R | PTE_W | PTE_V);

  // On mappe le PCIE
  map_page(root_pt_pa, PCI_ECAM_BASE_ADDRESS, PCI_ECAM_BASE_ADDRESS,
           PTE_R | PTE_W | PTE_V);

  // On mappe le CLINT
  map_page(root_pt_pa, CLINT_MSIP, CLINT_MSIP, PTE_R | PTE_X | PTE_W | PTE_V);

  // On mappe l'UART
  map_page(root_pt_pa, UART_BASE, UART_BASE, PTE_R | PTE_W | PTE_V);

  // On mappe le PLIC
  for (uintptr_t offset = 0; offset < 0x4000; offset++) {
    map_page(root_pt_pa, PLIC_SOURCE_BASE + (offset << 12),
             PLIC_SOURCE_BASE + (offset << 12), PTE_R | PTE_W | PTE_V);
  }
}

pagetable_t init_vm() {
  init_frames();
  pagetable_t root_pt_pa = (pagetable_t)get_frame();
  uint64_t root_pt_ppn = GET_PPN(PA2PTE(root_pt_pa));

  uint64_t sv39 = 8ULL << 60;
  root_pt_ppn = sv39 | root_pt_ppn;

  identity_map(root_pt_pa);

  __asm__("csrw satp, %0" ::"r"(root_pt_ppn));
  __asm__("sfence.vma zero, zero");

  return root_pt_pa;
}

// Descend les niveaux de la hiérarchie de la pagetable pour trouver la pa
// associée à l'adresse virtuelle va.
pte_t *walk(pagetable_t pagetable, uintptr_t va, int alloc) {
  // TODO: gestion de pte.A et pte.D

  for (int level = LEVELS - 1; level > 0; level--) {
    uint64_t vpn = GET_LINDEX(va, level);
    pte_t *pte = &pagetable[vpn];

    if (*pte & PTE_V) {
      // On descend dans l'arborescence
      pagetable = (pagetable_t)PTE2PA(*pte);
    } else {
      if (!alloc) {
        return NULL;
      }
      // On alloue une nouvelle page
      pagetable = (pagetable_t)get_frame();
      memset(pagetable, 0, PAGESIZE);
      *pte = PA2PTE(pagetable) | PTE_V;
    }
  }

  return &pagetable[GET_LINDEX(va, 0)];
}

void map_page(pagetable_t base, uintptr_t va, uintptr_t pa, uint8_t flags) {
  pte_t *pte = walk(base, va, 1);
  *pte = PA2PTE(pa) | flags;

  // On flushe le TLB
  __asm__("sfence.vma zero, zero");
}
