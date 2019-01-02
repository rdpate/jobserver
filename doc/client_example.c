#include "../lib/jobserver.h"

char const *commands[] = {
    "sleep 0.5; echo abc",
    "sleep 0.2; echo def",
    "sleep 0.4; echo ghi",
    "sleep 0.3; echo jkl",
    "sleep 0.0; echo mno",
    "sleep 0.1; echo xyz",
    0};

#include <unistd.h>
int main(int argc, char **argv) {
    //jobserver_init_or_exec(argv);
    jobserver_init_or_sync();

    for (char const **x = commands; *x; x ++) {
        //sleep(1);
        jobserver_bg_spawn("/bin/sh", "-c", *x, 0);
        }

    jobserver_exiting();
    return 0;
    }
