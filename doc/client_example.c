#include "../lib/jobserver.h"

char const *commands[] = {
    "sleep 0.6; echo abc",
    "sleep 0.3; echo def",
    "sleep 0.5; echo ghi",
    "sleep 0.4; echo jkl",
    "sleep 0.1; echo mno",
    "sleep 0.2; echo xyz",
    0};

#include <unistd.h>
int main(int argc, char **argv) {
    if (argc == 1) jobserver_init_or_exec(argv);
    else jobserver_init_or_sync();

    for (char const **x = commands; *x; x ++) {
        jobserver_bg_shell(*x, 0);
        }

    jobserver_exiting();
    return 0;
    }
