#include "user.h"
#include "assert.h"
#include "types.h"

// Doesn't handle times bigger than 2^31 - 1 because I don't feel like writing a
// new format specifier for printf at the moment.
uint64
current_timestamp(void)
{
    uint32 highOrder, lowOrder;
    asm ("rdtscp"
    // outputs to these registers, which are these variables:
        : "=d" (highOrder), "=a" (lowOrder)
    // takes no input registers:
        :
    // doesn't affect any other registers:
        :
    );
    
    return (((uint64) highOrder) << 32) | lowOrder;
}

int
main(int argc, char *argv[])
{
    if (argc == 1) {
        printf(1, "Usage: time program args...\n");
        exit();
    }

    char *program = argv[1];
    char **program_argv = argv + 1;

    uint64 start = current_timestamp();

    if (fork() == 0) {
        // In the child.
        if (exec(program, program_argv) == -1) {
            printf(2, "exec %s failed. Please ignore the following spurious timing message.\n", argv[1]);
        }
    } else {
        // In the parent.
        if (wait() == -1) {
            printf(2, "I just spawned a process, but apparently I have no kids. Why?\n");
            exit();
        }
        uint64 end = current_timestamp();
        printf(1, "%s took %ud cycles to run.\n", program, end - start);
    }
    exit();
}