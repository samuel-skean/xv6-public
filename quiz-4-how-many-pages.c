#include"types.h"
#include"user.h"
int main(int argc, char** argv) {
  int fd = open("LARGE",0);
  char* text = mmap(fd,1);

  if(text!=(void*)0x0000400000000000)
    printf(1,"Returned pointer is %p, should be 0x0000400000000000\n",text);
  printf(1,"%c\n",text[71661]); // Not even close to 2 meg, but let's see.

  exit();
}
