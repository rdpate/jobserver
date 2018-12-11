#include "common.inc.c"
#include "get_fds.inc.c"

static bool keep_stdin = false;
static int handle_option(char const *name, char const *value) {
    nonfatal("unknown option: %s (ignored)", name);
    return 64;
    }

// note: main must (attempt to) release a slot before exiting
int main(int argc, char **argv) {
    int rc;
    { // parse options
        char const *x = strrchr((prog_name = argv[0]), '/');
        if (x) prog_name = x + 1;
        char **rest = 0;
        rc = parse_options(argv + 1, &rest);
        if (rc)
            argc = 0;
        else {
            argc -= rest - argv;
            argv = rest;
            }
        }
    int write_fd = get_write_fd();
    if (argc) {
        nonfatal("unknown options/arguments (ignored)");
        rc = 64;
        }
    int _ = write(write_fd, "x", 1);
    return rc;
    }
