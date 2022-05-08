#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"
#include "traps.h"
#include "spinlock.h"

void pagefault_handler(struct trapframe *tf);

// Interrupt descriptor table (shared by all CPUs).
struct gatedesc idt[256];
extern uint vectors[];  // in vectors.S: array of 256 entry pointers
struct spinlock tickslock;
uint ticks;

void
tvinit(void)
{
  int i;

  for(i = 0; i < 256; i++)
    SETGATE(idt[i], 0, SEG_KCODE<<3, vectors[i], 0);
  SETGATE(idt[T_SYSCALL], 1, SEG_KCODE<<3, vectors[T_SYSCALL], DPL_USER);

  initlock(&tickslock, "time");
}

void
idtinit(void)
{
  lidt(idt, sizeof(idt));
}

//PAGEBREAK: 41
void
trap(struct trapframe *tf)
{
  if(tf->trapno == T_SYSCALL){
    if(myproc()->killed)
      exit();
    myproc()->tf = tf;
    syscall();
    if(myproc()->killed)
      exit();
    return;
  }

  switch(tf->trapno){
  case T_IRQ0 + IRQ_TIMER:
    if(cpuid() == 0){
      acquire(&tickslock);
      ticks++;
      wakeup(&ticks);
      release(&tickslock);
    }
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_IDE:
    ideintr();
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_IDE+1:
    // Bochs generates spurious IDE1 interrupts.
    break;
  case T_IRQ0 + IRQ_KBD:
    kbdintr();
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_COM1:
    uartintr();
    lapiceoi();
    break;
  case T_IRQ0 + 7:
  case T_IRQ0 + IRQ_SPURIOUS:
    cprintf("cpu%d: spurious interrupt at %x:%x\n",
            cpuid(), tf->cs, tf->eip);
    lapiceoi();
    break;
  case T_PGFLT:
    pagefault_handler(tf);
    break;
  //PAGEBREAK: 13
  default:
    if(myproc() == 0 || (tf->cs&3) == 0){
      // In kernel, it must be our mistake.
      cprintf("unexpected trap %d from cpu %d eip %x (cr2=0x%x)\n",
              tf->trapno, cpuid(), tf->eip, rcr2());
      panic("trap");
    }
    // In user space, assume process misbehaved.
    cprintf("pid %d %s: trap %d err %d on cpu %d "
            "eip 0x%x addr 0x%x--kill proc\n",
            myproc()->pid, myproc()->name, tf->trapno,
            tf->err, cpuid(), tf->eip, rcr2());
    myproc()->killed = 1;
  }

  // Force process exit if it has been killed and is in user space.
  // (If it is still executing in the kernel, let it keep running
  // until it gets to the regular system call return.)
  if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER)
    exit();

  // Force process to give up CPU on clock tick.
  // If interrupts were on while locks held, would need to check nlock.
  if(myproc() && myproc()->state == RUNNING &&
     tf->trapno == T_IRQ0+IRQ_TIMER)
    yield();

  // Check if the process has been killed since we yielded
  if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER)
    exit();
}

void pagefault_handler(struct trapframe *tf) {
  struct proc *curproc = myproc();
  void *fault_addr = (void *) rcr2();
  void *fault_page = (void *) PGROUNDDOWN((uint)fault_addr);

  cprintf("============in pagefault_handler============\n" \
          "pid %d %s: trap %d err %d on cpu %d " \
          "eip 0x%x addr 0x%x\n", \
          curproc->pid, curproc->name, tf->trapno, \
          tf->err, cpuid(), tf->eip, fault_addr);

  /*
    31              15                             4               0
    +---+--  --+---+-----+---+--  --+---+----+----+---+---+---+---+---+
    |   Reserved   | SGX |   Reserved   | SS | PK | I | R | U | W | P |
    +---+--  --+---+-----+---+--  --+---+----+----+---+---+---+---+---+
    P	1 bit	: Present	When set, the page fault was caused by a page-protection violation. When not set, it was caused by a non-present page.
    W	1 bit	: Write	When set, the page fault was caused by a write access. When not set, it was caused by a read access.
    U	1 bit	: User	When set, the page fault was caused while CPL = 3. This does not necessarily mean that the page fault was a privilege violation.    
  */
  if ((tf->err & 0x1) != 0) {
    /* Protection violation error. */
    // cprintf("XV6_TEST_OUTPUT : try to write to nonwritable page\n");
    curproc->killed = 1;
    return;
  }

  /* Validate that the faulting address has been allocated by this process */
  struct mmap_region *curr_region = curproc->mmap_regions;
  while (curr_region != (struct mmap_region *) 0) {
    /* Check the range of the region */
    if (((uint) fault_addr >= curr_region->start_addr) &&
         (uint) fault_addr < (curr_region->start_addr + curr_region->length)) {
      // cprintf("XV6_TEST_OUTPUT : found mmap region\n");
      break;
    }

    curr_region = curr_region->next_mmap_region;
  }

  /* Not a valid region so we kill the process */
  if (curr_region == (struct mmap_region *) 0) {
    curproc->killed = 1;
    // cprintf("XV6_TEST_OUTPUT : not valid mmap region\n");
    return;
  }

  /* Valid region so try to create a mapping in physical memory */
  char *mem = kalloc();
  if (mem == 0) {
    /* No more memory available */
    curproc->killed = 1;
    // cprintf("XV6_TEST_OUTPUT : no more memory\n");
    return;    
  }
  memset(mem, 0, PGSIZE);
  if (mappages(curproc->pgdir, fault_page, PGSIZE, (uint) V2P(mem), curproc->mmap_regions->prot | PTE_U) < 0) {
    kfree(mem);
    curproc->killed = 1;
    // cprintf("XV6_TEST_OUTPUT : unable to map regions\n");
    return;
  }
}
