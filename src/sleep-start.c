#include "common.h"
#include "get_fds.h"

static int handle_option(char const *name, char const *value, void *data) {
    nonfatal("unknown option %s (ignored)", name);
    return 64;
    }

// note: main must (attempt to) release a slot before exiting
int main_sleep_start(int argc, char **argv) {
    int rc = parse_options(&argc, &argv, &handle_option, NULL);
    int write_fd = get_write_fd();
    if (argc) {
        nonfatal("unknown options/arguments (ignored)");
        rc = 64;
        }
    if (write_all(write_fd, "x", 1) == -1) {
        perror("write");
        if (rc == 0) rc = 71;
        }
    return rc;
    }
