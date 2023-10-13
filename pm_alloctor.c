#include "types.h"
#include "defs.h"
#include "mmu.h"
#include "memlayout.h"



// the address after kernel loaded from ELF file
// defined by the kernel linker script in kernel.ld
extern char end[];

struct pfnode
{
    struct pfnode *next; 
};

struct {
    struct pfnode *freelist;
}kmem;



// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
char *kalloc(void)
{
    struct pfnode *pf;
    pf=kmem.freelist;
    if(pf)
    {
        kmem.freelist=pf->next;
    }
    return (char*)pf;

}



//free the page of physical memory pointed at p 
void kfree_one(char *v)
{
    struct pfnode *pf;
    // if align， or [end,PHYSTOP]
    if((unsigned int)v% PGSIZE || v <end || V2P(v) >= PHYSTOP)
    {
        //TODO: 
        // panic("free one page error...");
    }

    memset(v, 1,PGSIZE);
    //treate 8 bytes as pfnode
    pf=(struct pfnode*)v;
    pf->next=kmem.freelist;
    kmem.freelist=pf;
}

void free_range(void *vstart, void *vend)
{
    char *p;
    //4K align 
    p=(char*)PGROUNDUP((unsigned int)vstart);
    //manage 4K as a page 
    for(;p+PGSIZE<=(char*)vend;p+=PGSIZE){
        kfree_one(p);
    }
}



//still using entrypgdir
// first step: map [0-4m] physical mem
void
kinit1(void *vstart, void *vend)
{
  free_range(vstart, vend);
}