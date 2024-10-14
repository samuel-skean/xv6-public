#include "types.h"
#include "defs.h"
#include "param.h"
#include "traps.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "file.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"
#include "vga.h"
#define CRTPORT 0x3d4

static uint32 pixel_index = 0;
static unsigned char *const video_memory = (unsigned char*) P2V(0xA0000);
static ushort *const text_memory = (ushort*) P2V(0xb8000);
static ushort text_memory_backup[80*25];
static int text_cursor_pos_backup;

static void displayputp(unsigned char p) {
  video_memory[pixel_index] = p;
  pixel_index = (pixel_index + 1) % 64000;
}

int displayioctl(struct file *f, int param, int value) {
  switch (param){
  case 1:
    switch (value){
    case 0x13:
      memmove(text_memory_backup, text_memory, 80*25*sizeof(ushort));
      // Cursor position: col + 80*row.
      outb(CRTPORT, 14);
      text_cursor_pos_backup = inb(CRTPORT+1) << 8;
      outb(CRTPORT, 15);
      text_cursor_pos_backup |= inb(CRTPORT+1);
      vgaMode13();
      break;
    case 0x3:
      vgaMode3();
      outb(CRTPORT, 14);
      outb(CRTPORT+1, text_cursor_pos_backup>>8);
      outb(CRTPORT, 15);
      outb(CRTPORT+1, text_cursor_pos_backup);
      memmove(text_memory, text_memory_backup, 80*25*sizeof(ushort));
      break;
    default:
      return -1;
    }
    break;
  case 2:
    return -1; // TODO: Support palette changes.
    break;
  default:
    cprintf("Got unknown console ioctl request. %d = %d\n",param,value);
    return -1;
  }
  return 0;
}

int
displaywrite(struct file *f, char *buf, int n) {
  int i;
  for (i = 0; i < n; i++) {
    displayputp(buf[i]);
  }
  return i;
}

  void
displayinit(void) {
  devsw[DISPLAY].write = displaywrite;
  devsw[DISPLAY].read = 0;
  devsw[DISPLAY].ioctl = displayioctl;
}