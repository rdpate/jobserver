#include "common.h"
#include "get_fds.h"

int main_acquire(int argc, char **argv) {
    int rc = parse_no_options(&argc, &argv);
    if (rc) return rc;
    if (argc) fatal(64, "unexpected arguments");
    int read_fd = get_read_fd();
    ssize_t n;
    char c;
    while ((n = read(read_fd, &c, 1)) == -1) {
        switch (errno) {
            case EINTR:
                continue;
            default:
                perror("read");
                fatal(71, "read failed");
            }
        }
    if (n == 0) fatal(71, "unexpected EOF");
    return 0;
    }
