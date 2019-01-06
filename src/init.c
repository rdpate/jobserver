#include "common.h"
#include "get_fds.h"

int default_job_slots();
// defined in default_job_slots.c


static int const max_slots = 1000;

static bool reuse = true;
static int slots = 0;
static bool floating = true;

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
    ssize_t x = block_write(write_fd, "x", 1);
    if (x == -1) {
        perror("write");
        nonfatal("lost slot due to write error");
        return;
        }
    if (x == 1) {
        ++added;
        if (added == leak_warning) {
            nonfatal("probable job slot leak detected!");
            added /= 2;
            }
        return;
        }
    UNREACHABLE
    }
static void remove_slot() {
    char x;
    int rc = block_read(read_fd, &x, 1);
    if (rc == 1) {
        --added;
        return;
        }
    if (rc == 0) fatal(70, "unexpected EOF in remove_slot");
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

    execvp(argv[0], argv);
    perror("exec");
    return 65;
    }
static void no_op(int _) {}
static int do_float(pid_t child_pid) {
    { // setup SIGALRM handler
        struct sigaction act = {};
        act.sa_handler = no_op;
        if (sigaction(SIGALRM, &act, NULL) == -1) {
            perror("sigaction");
            fatal(71, "sigaction failed");
            }
        }
    // Check every 60 seconds.  If a change was required, then check again in 30 seconds.
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
        if (slot_change()) delay = 30;
        else delay = 60;
        }
    }

static int handle_option(char const *name, char const *value, void *_data) {
    if (strcmp(name, "new") == 0) {
        if (value) fatal(64, "unexpected value for option %s", name);
        reuse = false;
        }
    else if (strcmp(name, "slots") == 0) {
        if (!value || !*value) fatal(64, "missing or empty value for option %s", name);
        if (!to_int(&slots, value) || slots < 1)
            fatal(64, "invalid slots value %s", value);
        if (slots > max_slots) {
            nonfatal("capping slots at %d (instead of %d)", max_slots, slots);
            slots = max_slots;
            }
        }
    else if (strcmp(name, "no-float") == 0) {
        if (value) fatal(64, "unexpected value for option %s", name);
        floating = false;
        }
    else {
        fatal(64, "unknown option %s", name);
        }
    return 0;
    }

int main_init(int argc, char **argv) {
    int rc = parse_options(&argc, &argv, &handle_option, NULL);
    if (rc) return rc;
    if (argc == 0) fatal(64, "missing CMD argument");

    if (reuse && load_env_fds()) {
        if (slots != 0) nonfatal("ignoring explicit slots and reusing pool");
        execvp(argv[0], argv);
        perror("exec");
        fatal(71, "exec failed");
        }
    if (slots == 0) slots = default_job_slots();

    { // setup pipe
        int fds[2] = {-1, -1};
        if (pipe(fds)) {
            perror("pipe");
            fatal(71, "pipe failed");
            }
        read_fd = fds[0];
        write_fd = fds[1];
        int const start_fd = 100;
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
        if (fcntl(read_fd, F_SETFL, O_NONBLOCK) == -1) {
            perror("fcntl");
            return 71;
            }
        }
    { // add initial tokens
        --slots; // child will start with one
        while (slots) {
            int n = write_all(write_fd, "xxxxxxxxxxxxxxxx", (slots < 16 ? slots : 16));
            if (n == -1) {
                perror("write");
                fatal(70, "write failed");
            }
            else slots -= n;
            }
        }
    if (!floating) return do_exec(argv);
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
