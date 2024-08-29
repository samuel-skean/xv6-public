#include "user.h"
#include "assert.h"

int main(int argc, char *argv[])
{
    if (argc == 1) {
        printf(1, "Usage: time program args...\n");
        exit();
    }

    if (fork() == 0) {
        // In the child.
        if (exec(argv[1], argv + 1) == -1) {
            printf(2, "exec %s failed. Please ignore the following spurious timing message.\n", argv[1]);
        }
    } else {
        // In the parent.
        if (wait() == -1) {
            printf(2, "But we just *had* kids!\n");
            exit();
        }
        printf(1, "The child finished.\n");
    }
    exit();
}