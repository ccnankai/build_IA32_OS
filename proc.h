
// Per-CPU state
// Per-CPU state

struct cpu {
  unsigned char apicid;                // Local APIC ID
//   struct context *scheduler;   // swtch() here to enter scheduler
//   struct taskstate ts;         // Used by x86 to find stack for interrupt
  struct segdesc gdt[NSEGS];   // x86 global descriptor table
  volatile unsigned int started;       // Has the CPU started?
//   int ncli;                    // Depth of pushcli nesting.
//   int intena;                  // Were interrupts enabled before pushcli?
//   struct proc *proc;           // The process running on this cpu or null
};

extern struct cpu cpus[NCPU];
extern int ncpu;
