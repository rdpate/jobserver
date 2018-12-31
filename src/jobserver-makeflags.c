#include "common.h"
#include "get_fds.h"

static bool dup_fds = false;
static int handle_option(char const *name, char const *value, void *_data) {
    if (!strcmp(name, "dup")) {
        if (value) fatal(64, "unexpected value for option dup");
        dup_fds = true;
        }
    else fatal(64, "unknown option %s", name);
    return 0;
    }

static char const *makeflags_subtract() {
    char *makeflags = getenv("MAKEFLAGS");
    if (!makeflags) return "";
    char *end = makeflags + strlen(makeflags) + 1;
    char *start = makeflags;
    while ((start = strstr(start, " --jobserver-auth="))) {
        char const* next = strchr(start + 1, ' ');
        if (!next) {
            *start = '\0';
            break;
            }
        memmove(start, next, strlen(next) + 1);
        }
    start = makeflags;
    while ((start = strstr(start, " --jobserver-fds="))) {
        char const* next = strchr(start + 1, ' ');
        if (!next) {
            *start = '\0';
            break;
            }
        memmove(start, next, strlen(next) + 1);
        }
    start = makeflags + strlen(makeflags) + 1;
    memset(start, '\0', end - start);
    return makeflags;
    }
int main(int argc, char **argv) {
    int rc = main_parse_options(&argc, &argv, &handle_option, NULL);
    if (rc) return rc;

    int read_fd = get_read_fd();
    int write_fd = get_write_fd();
    if (dup_fds) {
        if (!argc) fatal(64, "cannot dup(2) without CMD");
        // dup(2) for MAKEFLAGS to prevent GNU Make from closing only copy
        int const start_fd = 100;
        if (read_fd < start_fd) {
            read_fd = fcntl(read_fd, F_DUPFD, start_fd);
            if (read_fd == -1) {
                perror("fcntl");
                fatal(69, "fcntl failed");
                }
            }
        if (write_fd < start_fd) {
            write_fd = fcntl(write_fd, F_DUPFD, start_fd);
            if (write_fd == -1) {
                perror("fcntl");
                fatal(69, "fcntl failed");
                }
            }
        }
    char const *makeflags;
    { // adjust MAKEFLAGS
        makeflags = makeflags_subtract();
        if (read_fd > 99999 || write_fd > 99999) return 70;
        char const *add = "MAKEFLAGS=%s -j --jobserver-auth=%d,%d --jobserver-fds=%d,%d";
        //                 1       10        20        30        40        50        60
        // increasing strlen(makeflags) by 70: 60 - 2 (%s) + 12 (4x %d, possible +3 each)
        char *new_flags = malloc(strlen(makeflags) + 70 + 1);
        if (!new_flags) {
            perror("malloc");
            fatal(70, "malloc failed");
            }
        sprintf(new_flags, add, makeflags, read_fd, write_fd, read_fd, write_fd);
        makeflags = new_flags;
        }
    if (!argc) {
        puts(strchr(makeflags, '=') + 1);
        return 0;
        }
    else {
        putenv((char *) makeflags);
        execvp(argv[0], argv);
        perror("exec");
        return 65;
        }
    }
