// DOCS:
// https://media.eyolfson.com/courses/utoronto/ece353/2024-winter/lecture-11.pdf
// https://cs326-s25.cs.usfca.edu/guides/page-tables

// TODO: Faire un unmap_pages

#include "vm.h"
#include "platform.h"
#include "pm.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>

extern char _kernel_start;
uintptr_t addr_kstart = (uintptr_t)&_kernel_start;
extern const uintptr_t memory_end; // Défini dans pm.c

// Mappe les adresses des périphériques (Écran, PCIE, CLINT, UART, PLIC) en 1:1
static void devices_mapping(pagetable_t root) {
  uintptr_t screen_baddr = BOCHS_DISPLAY_BASE_ADDRESS;
  uintptr_t screen_eaddr = BOCHS_DISPLAY_BASE_ADDRESS + DISPLAY_MEMORY_SIZE;

  for (uintptr_t addr = screen_baddr; addr < screen_eaddr; addr += PAGESIZE) {
    map_page(root, addr, addr, PTE_RWV);
  }

  // Ces périphériques ne prennent qu'une page (=4KB) en mémoire
  map_page(root, BOCHS_CONFIG_BASE_ADDRESS, BOCHS_CONFIG_BASE_ADDRESS, PTE_RWV);
  map_page(root, PCI_ECAM_BASE_ADDRESS, PCI_ECAM_BASE_ADDRESS, PTE_RWV);
  map_page(root, CLINT_MSIP, CLINT_MSIP, PTE_RWV);
  map_page(root, UART_BASE, UART_BASE, PTE_RWV);

  // NOTE: On mappe toutes les adresses du PLIC mais en pratique seul le
  // premier hart et les 2 contextes sont utilisés
  uintptr_t plic_baddr = PLIC_SOURCE_BASE;
  uintptr_t plic_eaddr = PLIC_SOURCE_BASE + PLIC_MEMORY_SIZE;
  for (uintptr_t addr = plic_baddr; addr < plic_eaddr; addr += PAGESIZE) {
    map_page(root, addr, addr, PTE_RWV);
  }
}

// Mappe les adresses 0x80000000-0xC0000000 en 1:1 ET 0x100000000-0x140000000
// (virtuelles) vers 0x80000000-0xC0000000 (physiques).
static void double_mapping(pagetable_t root) {
  for (uintptr_t addr = 0x80000000; addr < 0xC0000000; addr += PAGESIZE) {
    map_page(root, addr, addr, PTE_RWXV);
    // Autre mapping (2 va pointent donc vers la même pa)
    map_page(root, addr - 0x80000000 + 0x100000000, addr, PTE_RWXV);
  }
}

// Mappe les adresses de la RAM en 1:1
static void identity_mapping(pagetable_t root) {
  // On mappe la RAM. NOTE: le tas est mappé ici car il est compris dans la RAM
  for (uintptr_t addr = addr_kstart; addr < memory_end; addr += PAGESIZE) {
    map_page(root, addr, addr, PTE_RWXV);
  }
  // TODO: Mapper le kernel en lecture seule et le reste de la RAM en RWXV
  // Cela protégera la bitmap/freelist de pm.c
}

// Passe d'un adressage physique vers un adressage virtuel.
uint64_t init_vm(uint64_t asid) {
  // root n'est plus toujours une pa, ça l'est seulement pour idle
  pagetable_t root = (pagetable_t)get_frame();
  uint64_t root_ppn = GET_PPN(PA2PTE(root));

  // Les autres schemes (Bare, Sv48 et Sv57) ne sont pas pris en charge
  uint64_t sv39 = 8ULL << 60;
  uint64_t satp = sv39 | (asid << 44) | root_ppn;

  // On mappe l'adresse 0 vers une page n'ayant pas le flag valid, pour que
  // *NULL provoque une page fault
  map_page(root, 0, 0, 0);

  devices_mapping(root);
  identity_mapping(root);
  // double_mapping(root);

  return satp;
}

// Descend les niveaux de la hiérarchie de la pagetable pour trouver la pte
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

// Mappe une adresse virtuelle va à une adresse physique pa avec les flags
// donnés, dans la pagetable ayant pour adresse root.
void map_page(pagetable_t root, uintptr_t va, uintptr_t pa, uint8_t flags) {
  pte_t *pte = walk(root, va, 1);
  *pte = PA2PTE(pa) | flags;

  // On flushe dans le TLB la page d'adresse va pour tous les processus
  __asm__("sfence.vma %0, zero" ::"r"(va)); // PERF:
}
