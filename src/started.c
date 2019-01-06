#include "common.h"
#include "get_fds.h"

bool check(int fd) {
    if (fd <= 2) return false;
    struct stat x;
    if (fstat(fd, &x) == -1) {
        if (errno == EBADF) {
            // closed fd means not running (instead of an error)
            return false;
            }
        perror("fstat");
        fatal(70, "fstat failed");
        }
    return S_ISFIFO(x.st_mode);
    }

int main_started(int argc, char **argv) {
    int rc = parse_no_options(&argc, &argv);
    if (rc) return rc;
    if (argc) fatal(64, "unexpected arguments");

    if (!load_env_fds()) return 69;
    if (!check(get_read_fd())) return 69;
    if (!check(get_write_fd())) return 69;
    return 0;
    }
