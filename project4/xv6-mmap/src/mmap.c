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
    uint len;      // Length of mapping
    uint rtype;    // Region type. Anonymous or file backed
    uint offset;
    int fd;
    struct _mmap_node_ *prev;
    struct _mmap_node_ *next;
} mmap_node;

static mmap_node *PopList(mmap_node **list);
static void PushList(mmap_node **list, mmap_node *new_node);
static void RemoveFromList(mmap_node **list, mmap_node *node);

int sys_mmap(void) {
  /* Get the arguments */

  struct proc *p = myproc();
  mmap_node **list = (mmap_node **) &p->mmaps;
  mmap_node *new_node = (mmap_node *) kmalloc(sizeof(mmap_node));
  PushList(list, new_node);


  return 0;
}

int sys_munmap(void) {
  /* Get the arguments */
  char *addr, *len;
  argint(0, (int *) &addr);
  argint(1, (int *) &len);

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
    deallocuvm(p->pgdir, (uint) addr+PGSIZE, (uint) addr); //double check
    /* Remove the node */
    RemoveFromList(list, curr_node);
    kmfree(curr_node);
  }

  return 0;
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
  while(p->mmaps != NULL) {
    PopList(list);
  }
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