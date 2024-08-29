#include "types.h"
#include "x86.h"

int
main(void)
{
    // This idea is entirely courtesy of Jacob. He has yet to explain it to me.
    outw(0x604, 0x2000);
}