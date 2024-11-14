#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char *argv[]) {
  printf(1,"Freepages at start: %d\n",freepages());
  int* buf = malloc(10000000);
  memset(buf,0x88,10000000);
  printf(1,"Freepages after malloc: %d\n",freepages());
  dedup();
  for(int i=0;i<10000000/4;i++) {
    if(buf[i] != 0x88888888) {
      printf(2,"Argh, found an incorrect value in memory!\n");
      printf(2,"It was %x (first part of the next address) at address %p.\n", buf[i], &buf[i]);
      printf(2,"The thing after was %x at address %p (still part of the next address).\n", buf[i + 1], &buf[i + 1]);
      printf(2,"The thing after *that* was %x at address %p.\n", buf[i + 2], &buf[i + 2]);
      // If we failed to break one of the mappings, but still released it (and
      // freed it, in this case), we'll see the next free address (in 2 chunks,
      // because it's 8 bytes) because its now part of the free list, and then
      // alternating 0's and 1's (each hex digit only reflects 4 bits) because
      // kfree fills pages with 1s to make this easier to catch.
      exit();
    }
  }
  printf(1,"Freepages after: %d\n",freepages());
  exit();
}
