#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"

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

  struct proc *p = myproc();
  /* Get address */
  void *actual_addr;
  if (addr_hint >= (void *) KERNBASE || addr_hint < (void *) p->sz) {
    actual_addr = (void *) MMAPBASE;
  } else {
    actual_addr = (void *) PGROUNDUP((int) addr_hint);
  }

  //round up to the nearest page size
  len = ((len/PGSIZE) + 1) * PGSIZE;

  while((actual_addr + len) < (void *) KERNBASE &&
        allocuvm(p->pgdir, (uint) actual_addr, (uint) actual_addr + len) == 0) {
    actual_addr += len;
  }
  
  if (actual_addr < (void *) KERNBASE) {
    mmap_node **list = (mmap_node **) &p->mmaps;
    mmap_node *new_node = (mmap_node *) kmalloc(sizeof(mmap_node));
    if (new_node != NULL) {
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
  }

  // TODO: need to deallocate everything since there is an error?
  return -1;
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
      break;
    }

    if (curr_node->next == head) {
      curr_node = NULL;
    } else {
      curr_node = curr_node->next;
    }
  }

  /* Remvoe the mapping (deallocuvm) */
  if (curr_node != NULL) {
    deallocuvm(p->pgdir, (uint) curr_node->addr + curr_node->len, (uint) curr_node->addr); //double check
    /* Remove the node */
    RemoveFromList(list, curr_node);
    kmfree(curr_node);
    return 0;
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