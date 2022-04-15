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

int sys_mmap(void) {
  return 0;
}

int sys_munmap(void) {
  return 0;
}