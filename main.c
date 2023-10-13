#include "types.h"
#include "defs.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"


extern char end[]; // first address after kernel loaded from ELF file


int
main(void)
{
  kinit1(end,P2V(4*1024*1024));
  seginit();
}

__attribute__((__aligned__(PGSIZE)))
pde_t entrypgdir[NPDENTRIES] = {
  // Map VA's [0, 4MB) to PA's [0, 4MB)
  [0] = (0) | PTE_P | PTE_W | PTE_PS,
  // Map VA's [KERNBASE, KERNBASE+4MB) to PA's [0, 4MB)
  [KERNBASE>>PDXSHIFT] = (0) | PTE_P | PTE_W | PTE_PS,
};