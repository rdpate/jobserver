#include "common.h"
#include "get_fds.h"
#include "monitor.h"

int default_job_slots(void);
// defined in default_job_slots.c

static int const max_slots = 100;
    // must be less than pipe buffer size

static struct {
        int slots;
        bool reuse;
        bool monitor;
    } opts = {0, true, true};

static void do_exec(char **argv) {
    execvp(argv[0], argv);
    fatal(65, "exec: %s", strerror(errno));
    }
static void no_op(int _) {}
static int do_monitor(pid_t child_pid, int read_fd, int write_fd) {
    { // setup SIGALRM handler
        struct sigaction act = {};
        act.sa_handler = no_op;
        SYS( sigaction,(SIGALRM, &act, NULL) );
        }
    // Check every 60 seconds.  If a change was required, then check again in 30 seconds.
    int delay = 60;
    while (true) {
        alarm(delay);
        int status;
        pid_t rc = waitpid(child_pid, &status, 0);
        if (rc == -1) {
            if (errno != EINTR) fatal(71, "waitpid: %s", strerror(errno));
            }
        else if (rc == child_pid) {
            if (WIFEXITED(status)) return WEXITSTATUS(status);
            else if (WIFSIGNALED(status)) return WTERMSIG(status) + 128;
            }
        if (slot_change(read_fd, write_fd)) delay = 30;
        else delay = 60;
        }
    }

static void save_slots_value(char const *name, char const *value) {
    if (!*value) fatal(64, "empty slots value");
    if (!to_int(&opts.slots, value) || opts.slots < 1)
        fatal(64, "invalid slots value %s", value);
    if (opts.slots > max_slots) {
        nonfatal("capping slots at %d (instead of %d)", max_slots, opts.slots);
        opts.slots = max_slots;
        }
    }
static int handle_option(char const *name, char const *value, void *data) {
    if (strcmp(name, "new") == 0) {
        opts.reuse = false;
        if (value) save_slots_value(name, value);
        }
    else if (strcmp(name, "fixed") == 0) {
        opts.reuse = false;
        opts.monitor = false;
        if (value) save_slots_value(name, value);
        }
    else fatal(64, "unknown option %s", name);
    return 0;
    }
int main(int argc, char **argv) {
    int rc = main_parse_options(&argc, &argv, &handle_option, NULL);
    if (rc) return rc;
    if (argc == 0) fatal(64, "missing CMD argument");

    if (load_env_fds()) {
        if (opts.reuse) do_exec(argv);
        close(get_read_fd());
        close(get_write_fd());
        }
    if (opts.slots == 0) opts.slots = default_job_slots();

    int read_fd, write_fd;
    { // setup pipe
        int fds[2] = {-1, -1};
        SYS( pipe,(fds) );
        read_fd = fds[0];
        write_fd = fds[1];
        int const start_fd = 100;
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
        if (fcntl(read_fd, F_SETFL, O_NONBLOCK) == -1) {
            perror("fcntl");
            return 71;
            }
        }
    { // add initial tokens
        int slots = opts.slots;
        slots --; // child will start with one
        char const *x16 = "xxxxxxxxxxxxxxxx";
        while (slots) {
            int write_len = (slots < 16 ? slots : 16);
            int n;
            SYS( n = block_write,(write_fd, x16, write_len) );
            slots -= n;
            }
        }
    { // export JOBSERVER_FDS
        if (read_fd > 99999 || write_fd > 99999) return 70;
        char buffer[5+1+5+1];
        sprintf(buffer, "%d,%d", read_fd, write_fd);
        if (setenv("JOBSERVER_FDS", buffer, true) == -1) {
            perror("setenv");
            fatal(71, "setenv failed");
            }
        }
    if (!opts.monitor) do_exec(argv);
    pid_t pid;
    SYS( pid = fork,() );
    if (pid == 0) do_exec(argv);
    int leak_warning = opts.slots;
    if (leak_warning < INT_MAX / 2) leak_warning *= 2;
    init_monitor(opts.slots, leak_warning);
    return do_monitor(pid, read_fd, write_fd);
    }
