#include "types.h"
#include "x86.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "file.h"

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return proc->pid;
}

addr_t
sys_sbrk(void)
{
  addr_t addr;
  addr_t n;

  argaddr(0, &n);
  addr = proc->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(proc->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

#define MMAP_FAILED ((~0lu))

// user_page_start must be page-aligned. This could theoretically de-allocate
// unnecessary parts of the page-mapping structure, but that's hard :(, so I'm not. Places
// the physical address that was mapped to that page in *pa. That address is always page-aligned.
// Returns -1 on failure, 0 on success.
static int
unmappage(pml4e_t *pml4, void *user_page_start, addr_t *pa) 
{
  pte_t *pte;
  if ((pte = walkpgdir(pml4, user_page_start, 0)) == 0) {
    return -1;
  }
  if (!(*pte & PTE_P))
    panic("unmapped");
  *pte &= ~PTE_P; // Unsets the PTE_P bit.
  return *pte & ~PXMASK;
}

static addr_t
mmap_eager(struct inode *ip)
{
  ilock(ip);
  uint file_size = ip->size;
  addr_t address_of_map = proc->mmaptop;
  proc->mmaptop = PGROUNDUP(proc->mmaptop + file_size);
  for (addr_t uva = address_of_map; uva < address_of_map + file_size; uva += PGSIZE) {
    char *ka = kalloc();
    if (ka == 0){
      iunlock(ip);
      return MMAP_FAILED;
    }
    // The last read is likely to be a short read, since the file is unlikely
    // to have a size exactly a multiple of PGSIZE. This is fine.
    if (readi(ip, ka, uva - address_of_map, PGSIZE) == -1)
      panic("Failed to read");
      
    if (mappages(proc->pgdir, (void *) uva, PGSIZE, V2P(ka), PTE_W | PTE_U) < 0) {
      iunlock(ip);
      return MMAP_FAILED;
    }
  }
  iunlock(ip);

  lcr3(v2p(proc->pgdir)); // I don't think we need this, since we only *added* mappings, but let's do it just to be safe.
  return address_of_map;

 bad:
  
}

static addr_t
mmap_lazy(struct inode *ip, int fd) {
  if (proc->lazymmapcount == 10){
    return MMAP_FAILED;
  }
  ilock(ip);
  uint file_size = ip->size;
  iunlock(ip);

  addr_t address_of_map = (addr_t) proc->mmaptop;

  proc->lazymmaps[proc->lazymmapcount].fd = fd;
  proc->lazymmaps[proc->lazymmapcount].start = address_of_map;
  proc->lazymmapcount++;

  proc->mmaptop = PGROUNDUP(proc->mmaptop + file_size);

  return address_of_map;
}

  addr_t
sys_mmap(void)
{
  int fd, flags;
  struct file *f;
  if(argint(0,&fd) < 0 || argint(1,&flags) < 0)
    return MMAP_FAILED;
  if (fd < 0 || fd > NOFILE || (f = proc->ofile[fd]) == 0x0) // Heavily inspired by argfd
    return MMAP_FAILED;
  if (f->readable == 0 || f->type != FD_INODE)
    return MMAP_FAILED;

  switch (flags) {
    case 0: return mmap_eager(f->ip);
    case 1: return mmap_lazy(f->ip, fd);
    default: return MMAP_FAILED;
  }

}

  int
handle_pagefault(addr_t va)
{
  if (va < MMAPBASE || va >= proc->mmaptop)
    return 0; // Not an mmap.
  uint fd = NOFILE + 1;
  struct file *f;
  uint mmap_idx;
  // Find the lazy mmap by going backwards through the list of lazymmaps:
  for (mmap_idx = proc->lazymmapcount - 1; mmap_idx >= 0; mmap_idx--) {
    if (va >= proc->lazymmaps[mmap_idx].start) {
      fd = proc->lazymmaps[mmap_idx].fd;
      break;
    }
  }

  if (fd == NOFILE + 1) {
    cprintf("No file mmapped at that address.\n");
    return 0;
  }
  if ((f = proc->ofile[fd]) == 0x0) {
    cprintf("mmapped file was closed while mmapped.\n");
    return 0;
  }

  char *ka = kalloc();
  if (ka == 0){
    cprintf("Out of memory in mmap\n");
    return 0;
  }

  ilock(f->ip);

  addr_t user_page_start = PGROUNDDOWN((addr_t) va);
  // The last read is likely to be a short read, since the file is unlikely
  // to have a size exactly a multiple of PGSIZE. This is fine:
  if (readi(f->ip, ka, user_page_start - proc->lazymmaps[mmap_idx].start, PGSIZE) == -1) {
    cprintf("Failed to read\n");
    iunlock(f->ip);
    return 0;
  }
  iunlock(f->ip);
  if (mappages(proc->pgdir, (void *) user_page_start, PGSIZE, V2P(ka), PTE_W | PTE_U) < 0) {
    cprintf("Out of memory in mmap, specifically for the page tables.\n");
    return 0;
  }
  
  lcr3(v2p(proc->pgdir)); // I don't think we need this, since we only *added* mappings, but let's do it just to be safe.
  return 1;
}
