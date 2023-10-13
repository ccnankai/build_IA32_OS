
#include "param.h"
#include "types.h"
#include "defs.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"



// #include "defs.h"


// Set up CPU's kernel segment descriptors.
// each CPU run once
void seginit(void)
{
    struct cpu *c;

  // note:the CPU forbids an interrupt from CPL=0 to DPL=3.
  //TODO:
  c = &cpus[0];
  // c = &cpus[cpuid()];
  c->gdt[SEG_KCODE] = SEG(STA_X|STA_R, 0, 0xffffffff, 0);
  c->gdt[SEG_KDATA] = SEG(STA_W, 0, 0xffffffff, 0);
  c->gdt[SEG_UCODE] = SEG(STA_X|STA_R, 0, 0xffffffff, DPL_USER);
  c->gdt[SEG_UDATA] = SEG(STA_W, 0, 0xffffffff, DPL_USER);
  lgdt(c->gdt, sizeof(c->gdt));

}