#include "common.h"
#include "get_fds.h"
#include "monitor.h"

int default_job_slots(void);
// defined in default_job_slots.c

static int const max_slots = 100;
    // must be less than pipe buffer size

static struct {
        int slots;
        bool new_pool;
        bool fixed_pool;
        bool no_makeflags;
        bool attach;
    } opts = {};
int const start_fd = 100;
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
        opts.new_pool = true;
        if (value) save_slots_value(name, value);
        }
    else if (strcmp(name, "fixed") == 0) {
        opts.new_pool = true;
        opts.fixed_pool = true;
        if (value) save_slots_value(name, value);
        }
    else if (strcmp(name, "no-makeflags") == 0) {
        if (value) fatal(64, "unexpected value for option %s", name);
        opts.no_makeflags = true;
        }
    else if (strcmp(name, "-attach") == 0) {
        opts.attach = true;
        }
    else fatal(64, "unknown option %s", name);
    return 0;
    }

static void do_exec(char **argv) {
    execvp(argv[0], argv);
    fatal(65, "exec: %s", strerror(errno));
    }
static void no_op(int _) {}
static int do_monitor(pid_t child_pid, int read_fd, int write_fd) {
    // Check every 60 seconds.  If a change was required, then check again in 30 seconds.
    int const no_change_delay = 60;
    int const change_delay = 30;
    int delay = no_change_delay;

    if (!child_pid) {
        // --attach
        while (true) {
            sleep(delay);
            if (slot_change(read_fd, write_fd)) delay = change_delay;
            else delay = no_change_delay;
            }
        }

    { // setup SIGALRM handler
        struct sigaction act = {};
        act.sa_handler = no_op;
        SYS( sigaction,(SIGALRM, &act, NULL) );
        }
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
        if (slot_change(read_fd, write_fd)) delay = change_delay;
        else delay = no_change_delay;
        }
    }

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
    SYS( setenv,("MAKEFLAGS", makeflags, true) );
    free(makeflags);
    }
static void do_makeflags(int read_fd, int write_fd) {
    if (opts.no_makeflags) return;
    int make_read_fd, make_write_fd;
    SYSV( make_read_fd,  fcntl,(read_fd,  F_DUPFD, start_fd) );
    SYSV( make_write_fd, fcntl,(write_fd, F_DUPFD, start_fd) );
    set_makeflags(make_read_fd, make_write_fd);
    }

int main(int argc, char **argv) {
    int rc = main_parse_options(&argc, &argv, &handle_option, NULL);
    if (rc) return rc;

    if (opts.attach) {
        if (!load_env_fds()) fatal(70, "unexpected --attach problem");
        if (opts.slots == 0) opts.slots = default_job_slots();
        int leak_warning = opts.slots;
        if (leak_warning < INT_MAX / 2) leak_warning *= 2;
        init_monitor(opts.slots, leak_warning);
        return do_monitor(0, get_read_fd(), get_write_fd());
        }

    if (argc == 0) fatal(64, "missing CMD argument");
    if (load_env_fds()) {
        if (!opts.new_pool) {
            do_makeflags(get_read_fd(), get_write_fd());
            do_exec(argv);
            }
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
        if (read_fd < start_fd) {
            SYSV( read_fd, fcntl,(fds[0], F_DUPFD, start_fd) );
            close(fds[0]);
            }
        if (write_fd < start_fd) {
            SYSV( write_fd, fcntl,(fds[1], F_DUPFD, start_fd) );
            close(fds[1]);
            }
        }
    { // add initial tokens
        int slots = opts.slots;
        slots --; // child will start with one
        char const *x16 = "xxxxxxxxxxxxxxxx";
        while (slots) {
            int write_len = (slots < 16 ? slots : 16);
            int n;
            SYSV( n, block_write,(write_fd, x16, write_len) );
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
    do_makeflags(read_fd, write_fd);
    if (opts.fixed_pool) do_exec(argv);
    pid_t pid;
    SYSV( pid, fork,() );
    if (pid == 0) do_exec(argv);
    int leak_warning = opts.slots;
    if (leak_warning < INT_MAX / 2) leak_warning *= 2;
    init_monitor(opts.slots, leak_warning);
    return do_monitor(pid, read_fd, write_fd);
    }
