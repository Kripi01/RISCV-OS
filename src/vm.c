// DOCS:
// https://media.eyolfson.com/courses/utoronto/ece353/2024-winter/lecture-11.pdf
// https://cs326-s25.cs.usfca.edu/guides/page-tables

// Plan mémoire:
// On mappe le kernel en 1:1 entre 0x80000000 et 0xBFFFFFFF
// Un processsus est mappé dans l'espace virtuel à partir de 0x40000000
// La pile user est mappée à l'adresse virtuelle 0x50000000

// TODO: Ne mapper le kernel qu'une seule fois pour idle et ensuite juste copier
// les mappings pour les autres processus

// TODO: Quand j'aurai implémenté le mode user: freewalk pour libérer les page
// tables

#include "vm.h"
#include "platform.h"
#include "pm.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>

extern char _kernel_start;
uintptr_t kstart = (uintptr_t)&_kernel_start;
extern char _kernel_end;
uintptr_t kend = (uintptr_t)&_kernel_end;
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
  // Temporaire (pour le debug): UART accessible au mode U (et S grâce au SUM)
  map_page(root, UART_BASE, UART_BASE, PTE_RWV | PTE_U);

  // NOTE: On mappe toutes les adresses du PLIC mais en pratique seul le
  // premier hart et les 2 contextes sont utilisés
  uintptr_t plic_baddr = PLIC_SOURCE_BASE;
  uintptr_t plic_eaddr = PLIC_SOURCE_BASE + PLIC_MEMORY_SIZE;
  for (uintptr_t addr = plic_baddr; addr < plic_eaddr; addr += PAGESIZE) {
    map_page(root, addr, addr, PTE_RWV);
  }
}

// Mappe les adresses de la RAM en 1:1
static void identity_mapping(pagetable_t root) {
  // On mappe le kernel
  for (uintptr_t addr = kstart; addr < kend; addr += PAGESIZE) {
    map_page(root, addr, addr, PTE_RWXV);
  }

  // Le reste de la RAM (le tas par exemple) ne fait pas partie du kernel.
  for (uintptr_t addr = kend; addr < memory_end; addr += PAGESIZE) {
    map_page(root, addr, addr, PTE_RWXV);
  }
}

uint64_t init_vm(uint64_t asid) {
  pagetable_t root = (pagetable_t)get_frame(); // root est aligné sur 4KB
  uint64_t root_ppn = GET_PPN(PA2PTE(root));

  // Les autres schemes (Bare, Sv48 et Sv57) ne sont pas pris en charge
  uint64_t sv39 = 8ULL << 60;
  uint64_t satp = sv39 | (asid << 44) | root_ppn;

  // On mappe l'adresse 0 vers une page n'ayant pas le flag valid, pour que
  // *NULL provoque une page fault
  map_page(root, 0, 0, 0);

  devices_mapping(root);
  identity_mapping(root);
  return satp;
}

// Descend les niveaux de la hiérarchie de la pagetable pour renvoyer la pte
// associée à l'adresse virtuelle va.
pte_t *walk(pagetable_t pt, uintptr_t va, int alloc) {
  // TODO: gestion de pte.A et pte.D

  for (int level = LEVELS - 1; level > 0; level--) {
    uint64_t vpn = GET_LINDEX(va, level);
    pte_t *pte = &pt[vpn];

    if (*pte & PTE_V) {
      // On descend dans l'arborescence
      pt = (pagetable_t)PTE2PA(*pte);
    } else {
      // On alloue une nouvelle page
      pt = (pagetable_t)get_frame();
      if (!alloc || pt == NULL) {
        return NULL;
      }
      memset(pt, 0, PAGESIZE);
      *pte = PA2PTE(pt) | PTE_V;
    }
  }

  return &pt[GET_LINDEX(va, 0)];
}

// Mappe une adresse virtuelle va à une adresse physique pa avec les flags
// donnés, dans la pagetable ayant pour adresse root.
void map_page(pagetable_t root, uintptr_t va, uintptr_t pa, uint64_t flags) {
  pte_t *pte = walk(root, va, 1);
  if (pte != NULL) {
    *pte = PA2PTE(pa) | flags;

    // On flushe dans le TLB la page d'adresse va pour tous les processus
    __asm__("sfence.vma %0, zero" ::"r"(va)); // PERF:
  }
}
