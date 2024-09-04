#include "types.h"
#include "stat.h"
#include "user.h"

#include <stdarg.h>

static void
putc(int fd, char c)
{
  write(fd, &c, 1);
}

static char digits[] = "0123456789abcdef";

  static void
print_x64(int fd, addr_t x)
{
  int i;
  for (i = 0; i < (sizeof(addr_t) * 2); i++, x <<= 4)
    putc(fd, digits[x >> (sizeof(addr_t) * 8 - 4)]);
}

  static void
print_x32(int fd, uint x)
{
  int i;
  for (i = 0; i < (sizeof(uint) * 2); i++, x <<= 4)
    putc(fd, digits[x >> (sizeof(uint) * 8 - 4)]);
}

  static void
print_d(int fd, int v)
{
  char buf[16];
  int64 x = v;

  if (v < 0)
    x = -x;

  int i = 0;
  do {
    buf[i++] = digits[x % 10];
    x /= 10;
  } while(x != 0);

  if (v < 0)
    buf[i++] = '-';

  while (--i >= 0)
    putc(fd, buf[i]);
}

  static void print_ul(int fd, uint64 v)
{
  char buf[32];
  uint64 x = v;

  int i = 0;
  do {
    buf[i++] = digits[x % 10];
    x /= 10;
  } while (x != 0);

  while (--i >= 0)
    putc(fd, buf[i]);
}

// Print to the given fd. Only understands %d, %x, %p, %s.
  void
printf(int fd, char *fmt, ...)
{
  va_list ap;
  int i, c;
  char *s;

  va_start(ap, fmt);
  unsigned char un_signed = 0;

  for (i = 0; (c = fmt[i] & 0xff) != 0; i++) {
    if (c != '%') {
      putc(fd, c);
      continue;
    }
    c = fmt[++i] & 0xff;
    if (c == 0)
      break;
    switch(c) {
    case 'c':
      if (un_signed) {
        write(fd, "%uc not supported.\n", 19);
      } else {
        putc(fd, va_arg(ap, int));
      }
      break;
    case 'd':
      if (un_signed) {
        write(fd, "\n%ud not supported.\n", 20);
      } else {
        print_d(fd, va_arg(ap, int));
      }
      break;
    case 'u':
      un_signed = 1;
      continue;
    case 'l':
      if (un_signed) {
        print_ul(fd, va_arg(ap, uint64));
      } else {
        write(fd, "\n%l not supported.\n", 19);
      }
      break;
    case 'x':
      if (un_signed) {
        write(fd, "\n%ux not supported.\n", 20);
      } else {
        print_x32(fd, va_arg(ap, uint));
      }
      break;
    case 'p':
      if (un_signed) {
        write(fd, "\n%up not supported.\n", 20);
      } else {
        print_x64(fd, va_arg(ap, addr_t));
      }
      break;
    case 's':
      if (un_signed) {
        write(fd, "\n%us not supported.\n", 20);
        continue;
      }
      if ((s = va_arg(ap, char*)) == 0)
        s = "(null)";
      while (*s)
        putc(fd, *(s++));
      break;
    case '%':
      putc(fd, '%');
      break;
    default:
      // Print unknown % sequence to draw attention.
      putc(fd, '%');
      putc(fd, c);
      break;
    }
    un_signed = 0;
  }
}
