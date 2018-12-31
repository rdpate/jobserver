#include "common.h"
#include "get_fds.h"

static bool debug = false;
static int handle_option(char const *name, char const *value, void *data) {
    if (!strcmp(name, "debug")) {
        if (value) fatal(64, "unexpected value for option debug");
        debug = true;
        }
    else fatal(64, "unknown option %s", name);
    return 0;
    }

bool check(int fd) {
    if (fd <= 2) return false;
    struct stat x;
    if (fstat(fd, &x) == -1) {
        if (errno != EBADF) perror("fstat");
        return false;
        }
    return S_ISFIFO(x.st_mode);
    }

int main_started(int argc, char **argv) {
    int rc = parse_options(&argc, &argv, &handle_option, NULL);
    if (rc) return rc;
    if (argc) fatal(64, "unexpected arguments");

    if (!load_env_fds()) {
        if (debug) nonfatal("# jobserver not running!");
        return 69;
        }
    int read_fd = get_read_fd();
    int write_fd = get_write_fd();

    if (!check(read_fd) || !check(write_fd)) {
        if (debug) nonfatal("# jobserver not running!");
        return 69;
        }
    return 0;
    }
