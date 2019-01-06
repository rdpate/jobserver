
/*
* GNU Make 4.1 (and probably others) requires blocking FDs, as tested by:
    > mkfifo pipe
    > printf xx >>pipe &
    > exec 3<pipe 4>>pipe
    > export MAKE_FLAGS=' -j --jobserver-fds=3,4'
    > # Makefile contains 4 parallel "sleep 1" phony targets
    > time make
    > # set &3 non-blocking
    > time make
    > # make: *** read jobs pipe: Resource temporarily unavailable.  Stop.

* blocking IO complicates shared file descriptors under contention
    * if read_fd is ready, as indicated by poll or select, then immediately after, it may still block indefinitely

* Jobserver FDs are thus set non-blocking

* jobserver-makeflags creates a new (blocking) pipe and translates between it and the jobserver non-blocking pipe
    * the new pipe's FDs are saved in MAKEFLAGS
*/

#include "common.h"
#include "get_fds.h"

static void flag_subtract(char *value, char const *flag) {
    while ((value = strstr(value, flag))) {
        char const* next = strchr(value + 1, ' ');
        if (!next) {
            *value = '\0';
            break;
            }
        memmove(value, next, strlen(next) + 1);
        }
    }
static void set_makeflags(int read_fd, int write_fd) {
    if (read_fd < 0     || write_fd < 0    ) exit(70);
    if (read_fd > 99999 || write_fd > 99999) exit(70);
    char const *add = " -j --jobserver-auth=%d,%d --jobserver-fds=%d,%d";
    //                 1       10        20        30        40      48
    // increasing length by at most 60: 48 + 12 (4x %d, +3 each)
    // each %d is possible 5 chars output (given restriction to 99999)
    int const extra_len = 60;
    char *makeflags;
    int buffer_size;
    { // copy into new buffer
        char const *orig_makeflags = getenv("MAKEFLAGS");
        if (!orig_makeflags) orig_makeflags = "";
        buffer_size = strlen(orig_makeflags) + extra_len + 1;
        makeflags = malloc(buffer_size);
        if (!makeflags) {
            perror("malloc");
            exit(70);
            }
        strcpy(makeflags, orig_makeflags);
        }
    flag_subtract(makeflags, " --jobserver-fds=");
    flag_subtract(makeflags, " --jobserver-auth=");
    flag_subtract(makeflags, " -j");
    char *end = makeflags + strlen(makeflags);
    sprintf(end, add, read_fd, write_fd, read_fd, write_fd);
    setenv("MAKEFLAGS", makeflags, true);
    free(makeflags);
    }
int main(int argc, char **argv) {
    int rc = main_no_options(&argc, &argv);
    if (rc) return rc;
    if (!argc) fatal(64, "missing CMD argument");
    int jobsv[2] = {get_read_fd(), get_write_fd()};
    int make[2];
    if (pipe(make) == -1) {
        perror("pipe");
        return 71;
        }
    set_makeflags(make[0], make[1]);
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        return 71;
        }
    if (pid == 0) {
        // child
        execvp(argv[0], argv);
        perror("exec");
        _Exit(65);
        }
    // parent
    (void) jobsv;
    fatal(70, "TODO: figure out how to transfer slots between the queues");
    }
