#include "types.h"
#include "x86.h"

int
main(void)
{
    // This idea is courtesy of Jacob Cohen and the OSDev Wiki:
    // https://wiki.osdev.org/Shutdown
    // This only works in qemu, and I'm pretty surprised I can do it in
    // usermode. But at least I can regain access to my terminals this way!
    outw(0x604, 0x2000);
}