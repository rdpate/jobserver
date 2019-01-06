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
int main_acquire(int argc, char **argv) {
    int rc = parse_no_options(&argc, &argv);
    if (rc) return rc;
    if (argc) fatal(64, "unexpected arguments");
    int read_fd = get_read_fd();
    ssize_t n;
    char c;
    if ((n = block_read(read_fd, &c, 1)) == -1) {
        fatal(71, "read: %s", strerror(errno));
        }
    if (n == 0) fatal(71, "unexpected EOF");
    return 0;
    }

int main(int argc, char **argv) {
    prog_name = "jobsv-sh";
    --argc;
    ++argv;
    int rc = parse_no_options(&argc, &argv);
    if (rc) return rc;
    if (argc == 0) fatal(64, "missing subcommand argument");
#define SUB(NAME) do {                                                  \
    int main_##NAME(int, char **);                                      \
    if (strcmp(argv[0], #NAME) == 0) {                                  \
        prog_name = "jobsv-sh " #NAME;                                  \
        return main_##NAME(argc - 1, argv + 1);                         \
        }                                                               \
    } while (0);
    SUB(started)
    SUB(release)
    SUB(acquire)
    fatal(64, "unknown subcommand %s", argv[0]);
    }
