#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include <stdbool.h>

/*
|-------------------
|                   |
|                   |
|                   |
|                   |
|-------------------  KERNBASE
|                   |
|        MMAP       |
|-------------------  MMAPBASE
|                   |
|        HEAP       |
|-------------------  
|                   |
|     USER STACK    |
|-------------------  
|                   |
|     USER DATA     |
|-------------------  
|                   |
|     USER TEXT     |
|-------------------  
*/
/* Start allocating starting here. This is the base for the oldsz when calling allocuvms */
#define MMAPBASE 0x40000000
#define NULL 0

typedef struct _mmap_node_ {
    void *addr;    // Starting address
    void *hint_addr;
    int len;      // Length of mapping
    int prot;
    int flags;
    int rtype;    // TODO; Region type. Anonymous or file backed
    int offset;
    int fd;
    struct _mmap_node_ *prev;
    struct _mmap_node_ *next;
} mmap_node;

static mmap_node *PopList(mmap_node **list);
static void PushList(mmap_node **list, mmap_node *new_node);
static void RemoveFromList(mmap_node **list, mmap_node *node);
static void DestroyList(mmap_node **list);
extern pte_t *walkpgdir(pde_t *pgdir, const void *va, int alloc);

static bool is_already_mapped(pde_t *pgdir, void *start_addr, int len) {
  void *temp_addr = start_addr;
  while (temp_addr < start_addr + len) {
    pte_t *pte = walkpgdir(pgdir, temp_addr, 0);
    if (pte != 0 && ((*pte&PTE_P) != 0)) {
      /* Already mapped, so try to map to MMAPBASE (basically ignore the hint) */
      return true;
    }
    temp_addr += PGSIZE;
  }

  return false;
}

int sys_mmap(void) {
  /* Get the arguments */
  void *addr_hint;
  int len;
  int prot;
  int flags;
  int fd;
  int offset;
  if (argint(0, (int *) &addr_hint) < 0  ||
      argint(1, (int *) &len) < 0   ||
      argint(2, (int *) &prot) < 0  ||
      argint(3, (int *) &flags) < 0 ||
      argint(4, (int *) &fd) < 0    ||
      argint(5, (int *) &offset) < 0) {
       // TODO: need to deallocate everything since there is an error?
    return -1;
  }
  
  if (len <= 0) {
    // TODO: need to deallocate everything since there is an error?
    return -1;
  }

  //round up to the nearest page size
  len = ((len/PGSIZE) + 1) * PGSIZE;

  /* Get address */
  struct proc *p = myproc();
  void *actual_addr;
  if (addr_hint >= (void *) KERNBASE || addr_hint == NULL) {
    actual_addr = (void *) MMAPBASE;
  } else if (addr_hint < (void *) MMAPBASE) {
    /* Try to use the hint as an offset */
    void *temp_addr = (void *) PGROUNDUP((int) (MMAPBASE + addr_hint));
    if (((temp_addr + len) <= (void *)KERNBASE) &&
        !is_already_mapped(p->pgdir, temp_addr, len)) {
      actual_addr = temp_addr;
    } else {
      actual_addr = (void *) MMAPBASE;
    }
  } else {
    /* Try to use the address hint as the actual address. */
    void *temp_addr = (void *) PGROUNDUP((int) addr_hint);
    /* Check if available, else set the starting address to mmap base */
    if (!is_already_mapped(p->pgdir, temp_addr, len)) {
      actual_addr = temp_addr;
    } else {
      actual_addr = (void *) MMAPBASE;
    }
  }

  /* Find the block of virtual memory that has not been mapped and is large enough to fit requested size */
  void *start_addr = actual_addr;
  while ((start_addr + len) <= (void *) KERNBASE && is_already_mapped(p->pgdir, start_addr, len)) {
    actual_addr += len;
    start_addr = actual_addr;
  }

  if (start_addr + len > (void *) KERNBASE) {
    panic("mmap out of memory");
    // TODO: Error handling
    return -1;
  }

  if (allocuvm(p->pgdir, (uint) actual_addr, (uint) actual_addr + len) == 0) {
    // TODO: Error handling
    return -1;
  }
  
  mmap_node **list = (mmap_node **) &p->mmaps;
  mmap_node *new_node = (mmap_node *) kmalloc(sizeof(mmap_node));
  if (new_node == NULL) {
    // TODO: need to deallocate everything since there is an error?
    return -1;
  }
 
  new_node->addr = actual_addr;
  new_node->hint_addr = addr_hint;
  new_node->len = len;
  new_node->fd = -1;  //TODO: need to dup
  new_node->flags = flags; // TODO:
  new_node->prot = prot;  // TODO:
  new_node->rtype = -1;  //TODO: update this once we actually implement flags
  PushList(list, new_node);
  return (int) actual_addr;
}

int sys_munmap(void) {
  /* Get the arguments */
  void *addr;
  uint len;
  if (argint(0, (int *) &addr) < 0 || 
      argint(1, (int *) &len) < 0) {
        // TODO: need to deallocate everything since there is an error?
        return -1;
  }

  /* Find the mapping in the list */
  struct proc *p = myproc();
  mmap_node **list = (mmap_node **) &p->mmaps;
  mmap_node *head = *list;
  mmap_node *curr_node = head;
  while(curr_node != NULL) {
    if (curr_node->addr == addr) {  // still need to check length. not really necessary since we can assume it's always the same as when mmap was called
      memset(curr_node->addr, 0, curr_node->len); //Zero out the memory
      deallocuvm(p->pgdir, (uint) curr_node->addr + curr_node->len, (uint) curr_node->addr); //double check
      /* Remove the node */
      RemoveFromList(list, curr_node);
      kmfree(curr_node);
      return 0;
    }

    if (curr_node->next == head) {
      curr_node = NULL;
    } else {
      curr_node = curr_node->next;
    }
  }

  return -1;
}

void mmap_proc_init(struct proc *p) {
  p->mmaps = NULL;
}

void mmap_proc_deinit(void) {
  /* 
     Deallocate all the data structures used for the mappings
     No need to deallocate the memory aloocated in mmap because
     That is deallocated in freevm()
  */
  struct proc *p = myproc();
  mmap_node **list = (mmap_node **) &p->mmaps;
  DestroyList(list);
}

static mmap_node *PopList(mmap_node **list) {
  mmap_node *head = *list;

  if (head == NULL) {
    return head;
  }

  if (head->next == head) {
    head->next = NULL;
    head->prev = NULL;
    *list = NULL;
    return head;
  }

  mmap_node *old_tail = head->prev;
  head->prev = old_tail->prev;
  old_tail->prev->next = head;
  return old_tail;
}

/* Returns the head */
static void PushList(mmap_node **list, mmap_node *new_node) {
  mmap_node *head = *list;
  if (head == NULL) {
    new_node->prev = new_node;
    new_node->next = new_node;
    *list = new_node;
  } else {
    head->prev->next = new_node;
    new_node->prev = head->prev;
    new_node->next = head;
    head->prev = new_node;
  }
}

static void RemoveFromList(mmap_node **list, mmap_node *node) {
  mmap_node *head = *list;

  if (head == NULL) {
    return;
  }

  if (node->next == node) {
    /* Last node in list */
    node->next = NULL;
    node->prev = NULL;
    *list = NULL;
  } else {
    /* Patch it up */
    node->prev->next = node->next;
    node->next->prev = node->prev;
  }
}

static void DestroyList(mmap_node **list) {
  while (*list != NULL) {
    kmfree(PopList(list));
  }
}