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
static addr_t
mmap_eager(struct inode *ip)
{
  ilock(ip);
  uint file_size = ip->size;
  for (char *uva = proc->mmaptop; uva < proc->mmaptop + file_size; uva += PGSIZE) {
    char *ka = kalloc();
    if (ka == 0){
      iunlock(ip);
      panic("Out of memory in mmap"); // TODO: Handle gracefully.
    }
    if (readi(ip, ka, uva - proc->mmaptop, PGSIZE) == -1)
      panic("Failed to read"); // TODO: Handle gracefully. TODO: What should I do about short reads? Are those possible?
      // I think the last read is very likely to be a short read, since the file
      // is unlikely to have a size exactly a multiple of PGSIZE.
    if (mappages(proc->pgdir, uva, PGSIZE, V2P(ka), PTE_W | PTE_U) < 0) {
      iunlock(ip);
      panic("Out of memory in mmap, specifically for the page tables."); // TODO: Handle gracefully.
    }
  }
  iunlock(ip);

  addr_t address_of_map = (addr_t) proc->mmaptop;
  proc->mmaptop = (char *) PGROUNDUP((addr_t) proc->mmaptop + file_size);
  // TODO: Why does this work even without reloading %cr3?
  return address_of_map;
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

  proc->mmaptop = (char *) PGROUNDUP((addr_t) proc->mmaptop + file_size);

  // TODO: Why does this work even without reloading %cr3?
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
    case 1: return mmap_lazy(f->ip, fd); // TODO: Make this lazy.
    default: return MMAP_FAILED;
  }

}

  int
handle_pagefault(addr_t va)
{
  uint fd = NOFILE + 1;
  struct file *f;
  uint mmap_idx;
  // BUG EXPLANATION: This goes forward through the list of mmaps, and finds the
  // first one where the address we faulted on is greater than it's start.
  // Unfortunately, if we are accessing the address of a lazy mmap and we're
  // faulting, that address is *always* greater than the start address of the
  // first lazy mmap because the mmaps for a process always increase in user
  // address through the list of mmaps. So, we always get the first lazy mmap
  // even if that's not the mmap we're accessing.
  for (mmap_idx = 0; mmap_idx < proc->lazymmapcount; mmap_idx++) {
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

  addr_t uva_page_start = PGROUNDDOWN((addr_t) va);
  // BUG EXPLANATION: Interestingly, we fail on readi - before we even try to
  // map in the file at the wrong spot (in the user program, that would be
  // somewhere already mapped). That's because the offset is calculated from the
  // user virtual address compared to that start of the original mmap, and that
  // offset is *always* greater than the size of the file if we've got the wrong
  // mmap, causing readi to fail.
  if (readi(f->ip, ka, uva_page_start - proc->lazymmaps[mmap_idx].start, PGSIZE) == -1) {
    cprintf("Failed to read\n"); // TODO: Handle short reads, or change comment.
    iunlock(f->ip);
    return 0;
  }
  iunlock(f->ip);
  if (mappages(proc->pgdir, uva_page_start, PGSIZE, V2P(ka), PTE_W | PTE_U) < 0) {
    cprintf("Out of memory in mmap, specifically for the page tables.\n");
    return 0;
  }
  
  // lcr3(v2p(proc->pgdir)); // TODO: Why isn't this necessary?
  return 1;
}
