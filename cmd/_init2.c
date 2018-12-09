#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
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

static bool to_int(int *dest, char const *s) {
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
static int slot_adjustment() {
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
static bool slot_change() {
    // Perform slot adjustment; return whether adjustment was required.
    int adj = slot_adjustment();
    if (!adj) return false;
    if (adj < 0) remove_slot();
    else add_slot();
    return true;
    }

static bool start_float = true;
static int handle_option(char const *name, char const *value) {
    if (!strcmp(name, "no-float")) {
        if (value) fatal(64, "unexpected value for option no-float");
        start_float = false;
        }
    else fatal(64, "unknown option: %s", name);
    return 0;
    }
static int parse_options(char **args, char ***rest) {
    // modifies argument for long options with values, though does reverse the modification afterwards
    // returns 0 or the first non-zero return from handle_option
    // if rest is non-null, *rest is the first unprocessed argument, which will either be the first non-option argument or the argument for which handle_option returned non-zero
    int rc = 0;
    for (; args[0]; ++args) {
        if (!strcmp(args[0], "--")) {
            args += 1;
            break;
            }
        else if (args[0][0] == '-' && args[0][1] != '\0') {
            if (args[0][1] == '-') {
                // long option
                char *eq = strchr(args[0], '=');
                if (!eq) {
                    // long option without value
                    if ((rc = handle_option(args[0] + 2, NULL))) {
                        break;
                        }
                    }
                else {
                    // long option with value
                    *eq = '\0';
                    if ((rc = handle_option(args[0] + 2, eq + 1))) {
                        *eq = '=';
                        break;
                        }
                    *eq = '=';
                    }
                }
            else if (args[0][2] == '\0') {
                // short option without value
                if ((rc = handle_option(args[0] + 1, NULL))) {
                    break;
                    }
                }
            else {
                // short option with value
                char name[2] = {args[0][1]};
                if ((rc = handle_option(name, args[0] + 2))) {
                    break;
                    }
                }
            }
        else break;
    }
    if (rest) {
        *rest = args;
        }
    return rc;
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
static int do_exec(char **argv) {
    if (read_fd > 99999 || write_fd > 99999) return 70;

    { // export JOBSERVER_FDS
        int newlen = 14 +           5 + 1 + 5 + 1;
        // strlen("JOBSERVER_FDS=") %d  ',' %d  '\0'
        char *var = malloc(newlen);
        if (!var) {
            perror("malloc");
            fatal(70, "malloc failed");
            }
        sprintf(var, "JOBSERVER_FDS=%d,%d", read_fd, write_fd);
        putenv(var);
        }

    { // export MAKEFLAGS, after adjustment
        char const *makeflags = makeflags_subtract();
        char const *add = "MAKEFLAGS=%s -j --jobserver-auth=%d,%d --jobserver-fds=%d,%d";
        //                 1       10        20        30        40        50        60
        // increasing strlen(makeflags) by 70: 60 - 2 (%s) + 12 (4x %d, possible +3 each)
        char *new_flags = malloc(strlen(makeflags) + 70 + 1);
        if (!new_flags) {
            perror("malloc");
            fatal(70, "malloc failed");
            }
        sprintf(new_flags, add, makeflags, read_fd, write_fd, read_fd, write_fd);
        putenv(new_flags);
        }

    execvp(argv[0], argv);
    perror("exec");
    return 65;
    }
static void noop(int _) {}
static int do_float(pid_t child_pid) {
    { // setup SIGALRM handler
        struct sigaction act = {};
        act.sa_handler = noop;
        if (sigaction(SIGALRM, &act, NULL) == -1) {
            perror("sigaction");
            fatal(71, "sigaction failed");
            }
        }
    // "Preload" one slot to help with initial burst of activity which will not be immediately reflected in loadavg, for cases when started for a specific, intensive task that (hopefully) doesn't last long, like compiling.
    add_slot();
    // Then start checking regularly.
    int delay = 60;
    while (true) {
        alarm(delay);
        int status;
        pid_t rc = waitpid(child_pid, &status, 0);
        if (rc == -1) {
            if (errno != EINTR) {
                perror("waitpid");
                fatal(71, "waitpid failed");
                }
            }
        else if (rc == child_pid) {
            if (WIFEXITED(status)) return WEXITSTATUS(status);
            else if (WIFSIGNALED(status)) return WTERMSIG(status) + 128;
            }
        if (slot_change()) delay = 10;
        else delay = 60;
        }
    }
int main(int argc, char **argv) {
    { // parse options
        char const *x = strrchr((prog_name = argv[0]), '/');
        if (x) prog_name = x + 1;
        char **rest = 0;
        int rc = parse_options(argv + 1, &rest);
        if (rc) return rc;
        argc -= rest - argv;
        argv = rest;
        }
    // % SLOTS CMD..
    if (argc < 2) fatal(70, "not enough arguments");
    { // setup pipe
        int fds[2] = {-1, -1};
        if (pipe(fds)) {
            perror("pipe");
            fatal(71, "pipe failed");
            }
        read_fd = fds[0];
        write_fd = fds[1];
        int const start_fd = 0; // once rewritten without shell, set to 100
        if (start_fd) {
            if (read_fd < start_fd) {
                read_fd = fcntl(fds[0], F_DUPFD, start_fd);
                if (read_fd == -1) {
                    perror("fcntl");
                    fatal(69, "fcntl failed");
                    }
                close(fds[0]);
                }
            if (write_fd < start_fd) {
                write_fd = fcntl(fds[1], F_DUPFD, start_fd);
                if (write_fd == -1) {
                    perror("fcntl");
                    fatal(69, "fcntl failed");
                    }
                close(fds[1]);
                }
            }
        }
    int slots;
    if (!to_int(&slots, argv[0]) || slots < 1)
        fatal(70, "expected first argument to be a number >= 1");
    ++argv, --argc;
    { // add initial tokens
        --slots; // child will start with one
        while (slots) {
            int n = write(write_fd, "xxxxxxxxxxxxxxxx", (slots < 16 ? slots : 16));
            if (n == -1 && errno != EINTR) {
                perror("write");
                fatal(70, "write failed");
            }
            slots -= n;
            }
        }
    if (!start_float) return do_exec(argv);
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        fatal(71, "fork failed");
        }
    if (pid == 0) {
        // child
        return do_exec(argv);
        }
    // parent
    set_target(slots);
    leak_warning = slots;
    if (leak_warning < INT_MAX / 2) leak_warning *= 2;
    return do_float(pid);
    }
