#include "param.h"
#include "types.h"
#include "defs.h"
#include "x86.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "elf.h"

//in kernel.ld
extern char data[];
// use in scheduler()
pde_t *kpgdir;

// Set up CPU's kernel segment descriptors.
// each CPU run once
void seginit(void)
{
    struct cpu *c;

  // note:the CPU forbids an interrupt from CPL=0 to DPL=3.
  c = &cpus[cpuid()];
  c->gdt[SEG_KCODE] = SEG(STA_X|STA_R, 0, 0xffffffff, 0);
  c->gdt[SEG_KDATA] = SEG(STA_W, 0, 0xffffffff, 0);
  c->gdt[SEG_UCODE] = SEG(STA_X|STA_R, 0, 0xffffffff, DPL_USER);
  c->gdt[SEG_UDATA] = SEG(STA_W, 0, 0xffffffff, DPL_USER);
  lgdt(c->gdt, sizeof(c->gdt));

}



static pte_t *
walkpgdir(pde_t *pgdir, const void *va, int alloc)
{
  // a item in pgdir
  pde_t *pde;
  //page talbe virtual address
  pte_t *pgtab;

  pde =&pgdir[PDX(va)];
  //if pte exists
  if(*pde &PTE_P)
  {
    pgtab=(pte_t *)P2V(PTE_ADDR(*pde));
  }
  else
  {
    if(!alloc ||(pgtab =(pte_t *)kalloc())==0)
    {
      return 0;
    }
    
    memset(pgtab,0,PGSIZE);
    *pde = V2P(pgtab) | PTE_P | PTE_W | PTE_U;
  }
  
  return &pgtab[PTX(va)];
}



//file in the physical address in pte
static int
mappages(pde_t *pgdir, void *va, uint size, uint pa, int perm)
{
  char *a , *last;
  pte_t *pte;

  a=(char *)PGROUNDDOWN((uint)va);
  last=(char *)PGROUNDDOWN(((uint)va) + size - 1);
  for(;;)
  {
    if((pte =walkpgdir(pgdir,a,1))==0)
    {
      return -1;
    }
    if(*pte & PTE_P)
    {
      panic("remap");
    }
    *pte = pa| perm |PTE_P;
    if(a==last) break;
    a+=PGSIZE;
    pa +=PGSIZE;
  }

  return 0;
}

static struct kmap {
  void *virt;
  uint phys_start;
  uint phys_end;
  int perm;
} kmap[] = {
 { (void*)KERNBASE, 0,             EXTMEM,    PTE_W}, // I/O space
 { (void*)KERNLINK, V2P(KERNLINK), V2P(data), 0},     // kern text+rodata
 { (void*)data,     V2P(data),     PHYSTOP,   PTE_W}, // kern data+memory
 { (void*)DEVSPACE, DEVSPACE,      0,         PTE_W}, // more devices
};


//typedef unsigned int
pde_t*
setupkvm(void)
{
  pde_t *pgdir;
  struct kmap *k;

  if((pgdir = (pde_t*)kalloc())==0)
    return 0;
  memset(pgdir,0,PGSIZE);
  if (P2V(PHYSTOP) > (void*)DEVSPACE)
  panic("PHYSTOP too high");
  
  for(k=kmap;k<&kmap[NELEM(kmap)];++k)
  {
    if(mappages(pgdir, k->virt, k->phys_end - k->phys_start,
                (uint)k->phys_start, k->perm) < 0) {
      freevm(pgdir);
      return 0;
    }

  }
    return pgdir;  
}

//Allocate one page table for the kernel address space for scheduler process.
void 
kvmalloc(void)
{
  kpgdir=setupkvm();
  switchkvm();
}

// Switch h/w page table register to the kernel-only page table,
// for when no process is running.
void
switchkvm(void)
{
  lcr3(V2P(kpgdir));   // switch to the kernel page table
}


//user space 
int
allocuvm(pde_t *pgdir, uint oldsz, uint newsz)
{
  char *mem;
  uint a;

  if(newsz >= KERNBASE)
    return 0;
  if(newsz < oldsz)
    return oldsz;

  a = PGROUNDUP(oldsz);
  for(; a < newsz; a += PGSIZE){
    mem = kalloc();
    if(mem == 0){
      //TODO:
      // cprintf("allocuvm out of memory\n");
      deallocuvm(pgdir, newsz, oldsz);
      return 0;
    }
    memset(mem, 0, PGSIZE);
    if(mappages(pgdir, (char*)a, PGSIZE, V2P(mem), PTE_W|PTE_U) < 0){
      //TODO:
      // cprintf("allocuvm out of memory (2)\n");
      deallocuvm(pgdir, newsz, oldsz);
      kfree(mem);
      return 0;
    }
  }
  return newsz;
}


//user space
int
deallocuvm(pde_t *pgdir, uint oldsz, uint newsz)
{
  pte_t *pte;
  uint a, pa;

  if(newsz >= oldsz)
    return oldsz;

  a = PGROUNDUP(newsz);
  for(; a  < oldsz; a += PGSIZE){
    //find  pte virtual address
    pte = walkpgdir(pgdir, (char*)a, 0);
    if(!pte)
    //generate virutal address according pde item shift , pte item shift....
      a = PGADDR(PDX(a) + 1, 0, 0) - PGSIZE;
    else if((*pte & PTE_P) != 0){
      //find physical address store in the pte item , first 20bit [total 32bit]
      pa = PTE_ADDR(*pte);
      if(pa == 0)
        panic("kfree");
      char *v = P2V(pa);
      kfree(v);
      *pte = 0;
    }
  }
  return newsz;
}


//Free a  page table and all physicall memory pages in the user part
void
freevm(pde_t *pgdir)
{
  uint i;

  if(pgdir == 0)
    panic("freevm: no pgdir");
  deallocuvm(pgdir, KERNBASE, 0);
  for(i = 0; i < NPDENTRIES; i++){
    //if pde exist
    if(pgdir[i] & PTE_P){
      //find pte physical addr ,and convert to pte virtual address 
      char * v = P2V(PTE_ADDR(pgdir[i]));
      kfree(v);
    }
  }
  //pgdir ptab are both 4k ,one page ,so  kfree
  kfree((char*)pgdir);
}