#include <limits.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

static char const *prog_name = "<unknown>";
void fatal(int rc, char const *fmt, ...) {
    fprintf(stderr, "%s error: ", prog_name);
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fputc('\n', stderr);
    exit(rc);
    }
void nonfatal(char const *fmt, ...) {
    fprintf(stderr, "%s: ", prog_name);
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fputc('\n', stderr);
    }

bool to_int(int *dest, char const *s) {
    char const *end;
    errno = 0;
    long temp = strtol(s, (char **)&end, 10);
    if (end == s) errno = EINVAL;
    else if (!errno) {
        if (INT_MIN <= temp && temp <= INT_MAX) {
            *dest = temp;
            return true;
            }
        errno = ERANGE;
        }
    return false;
    }

static double target_begin, target_load, target_end;
static void set_target(double target) {
    // target load is a range
    target_begin  = target - 0.5;
    target_load   = target;
    target_end    = target + 0.5;
    }
static int added, leak_warning, read_fd, write_fd;
static bool slots_are_waiting() {
    struct pollfd x = {read_fd, POLLIN};
    int rc = poll(&x, 1, 0);
    if (rc == -1) {
        perror("poll");
        return false;
        }
    if (rc == 0) return false;
    return (x.revents & POLLIN);
    }
static int slot_change() {
    double current = target_load;
    getloadavg(&current, 1);
    if (target_end < current) {
        // need to reduce load
        return added ? -1 : 0;
        }
    if (current < target_begin) {
        // either children are being selfish...
        if (!slots_are_waiting()) {
            // I'm givin' her all she's got, Captain!
            return 1;
            }
        // ...or currently lacking work
        return added ? -1 : 0;
        }
    return 0;  // currently within Goldilocks Zone
    }
static int read_fd, write_fd;
static bool setup_fds() {
    char *s = getenv("JOBSERVER_FDS");
    if (!s || !*s) return false;
    char *read = s;
    while (*s && *s != ',') ++s;
    if (!s) return false;
    char *write = s + 1;
    *s = '\0';
    bool read_ok   = to_int(&read_fd,   read);
    bool write_ok  = to_int(&write_fd,  write);
    *s = ',';
    return read_ok && write_ok && read_fd >= 0 && write_fd >= 0;
    }
static void add_slot() {
    //puts("DEBUG: add_slot");
    ssize_t x = write(write_fd, "x", 1);
    if (x == 1) {
        ++added;
        if (added == leak_warning) {
            nonfatal("probable job slot leak detected!");
            added /= 2;
            }
        return;
        }
    if (x == 0) fatal(70, "TODO: can this ever happen?");
    if (errno == EAGAIN || errno == EWOULDBLOCK)
        fatal(69, "jobserver descriptor changed to non-blocking mode!");
    perror("write");
    }
static void remove_slot() {
    //puts("DEBUG: remove_slot");
    char x;
    int rc = read(read_fd, &x, 1);
    if (rc == 1) {
        --added;
        return;
        }
    if (rc == 0) fatal(70, "TODO: can this ever happen?");
    if (errno == EAGAIN || errno == EWOULDBLOCK)
        fatal(69, "jobserver descriptor changed to non-blocking mode!");
    perror("read");
    }

void noop(int _) {}

int main(int argc, char **argv) {
    { // prog_name
        char const *x = strrchr((prog_name = argv[0]), '/');
        if (x) prog_name = x + 1;
        }
    if (argc < 3) fatal(70, "not enough arguments");
    if (!setup_fds()) fatal(70, "cannot get file descriptors");

    int n;
    if (!to_int(&n, argv[2]) || n < 0)
        fatal(70, "expected arg 2 to be a (non-negative) number");
    set_target(n);
    leak_warning = n;
    if (leak_warning < INT_MAX / 2) leak_warning *= 2;

    { // setup alarm signal handler
        struct sigaction act = {};
        act.sa_handler = noop;
        if (sigaction(SIGALRM, &act, NULL) == -1) {
            perror("sigaction");
            return 70;
            }
        }

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        fatal(65, "fork failed");
        }
    if (pid == 0) {
        // child
        execvp(argv[1], argv + 1);
        perror("exec");
        fatal(65, "exec failed");
        }
    // parent

    // "Preload" one slot to help with initial burst of activity which will not be immediately reflected in loadavg, for cases when started for a specific, intensive task that (hopefully) doesn't last long, like compiling.
    add_slot();
    // Then start checking regularly.
    int delay = 60;
    while (true) {
        alarm(delay);
        int status;
        pid_t rc = waitpid(pid, &status, 0);
        if (rc == -1) {
            if (errno != EINTR) {
                perror("waitpid");
                return 70;
                }
            }
        else if (rc == pid) {
            if (WIFEXITED(status)) return WEXITSTATUS(status);
            else if (WIFSIGNALED(status)) return WTERMSIG(status) + 128;
            }
        int change = slot_change();
        if (change == 0)
            delay = 60;
        else {
            delay = 10;
            if (change > 0) add_slot();
            else remove_slot();
            }
        }
    }
