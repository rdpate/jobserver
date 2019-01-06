// internal details
    #include "jobserver.h"
    #include <errno.h>
    #include <limits.h>
    #include <signal.h>
    #include <stdarg.h>
    #include <stdio.h>
    #include <stdlib.h>
    #include <time.h>

    #include <fcntl.h>
    #include <sys/signalfd.h>
    #include <sys/time.h>
    #include <sys/types.h>
    #include <sys/wait.h>
    #include <unistd.h>

    static struct {
        int read_fd, write_fd;
        int slots_held, children;
        int devnull;

        int child_fd;
        timer_t nonblock_timer;
        jobserver_wait_callback wait_callback;
        void *wait_callback_data;
        } self;
    static struct itimerspec const safe_delay = {{}, {0, 10 * 1000}};
        // milliseconds afterwhich to EINTR read()
    static struct itimerspec const no_delay = {};

    //#define log_msg(msg)            jobserver_error(0, __func__, 0, msg)
    //#define fatal_func(func)        jobserver_error(70, func, errno, NULL)
    #define fatal_sysfunc(func)     jobserver_error(71, func, errno, NULL)
    #define fatal_usage(msg)        jobserver_error(70, __func__, 0, msg)
    #define check_init_return(rc) do { \
        if (!self.write_fd) { \
            fatal_usage("must jobserver_init first"); \
            return rc; \
            } \
        } while (0)
// High-level Interface
    bool jobserver_init_or_exec(char **self_args) {
        if (!self_args || !*self_args) {
            fatal_usage("bad self_args");
            return false;
            }
        if (jobserver_init()) return true;
        if (errno != 0) return false;
        int argc = 4 + 1;
        for (char **x = self_args + 1; *x; x ++) argc ++;
        char *argv[argc];
        char **n = argv + 3;
        argv[0] = "jobserver";
        argv[1] = "init";
        argv[2] = "--";
        for (char **x = self_args; *x; x ++) {
            *n = *x;
            n ++;
            }
        *n = 0;
        execvp(argv[0], argv);
        fatal_sysfunc("exec");
        return false;
        }
    bool jobserver_init_or_sync(jobserver_wait_callback func, void *data) {
        if (jobserver_init()) return true;
        if (errno != 0) return false;
        int p[2];
        if (pipe2(p, O_CLOEXEC) == -1) {
            fatal_sysfunc("pipe2");
            return false;
            }
        if (unsetenv("JOBSERVER_FDS") == -1) {
            fatal_sysfunc("unsetenv");
            return false;
            }
        self.read_fd = p[0];
        self.write_fd = p[1];
        self.slots_held = 1;
        return true;
        }
    static int spawn(void *data) {
        char **args = data;
        execvp(args[0], args);
        return 71;
        }
    pid_t jobserver_bg(int (*func)(void *data), void *data) {
        check_init_return((pid_t) -1);
        pid_t pid = fork();
        if (pid == -1) {
            fatal_sysfunc("fork");
            return pid;
            }
        if (pid == 0) {
            // child
            jobserver_forked_child();
            if (!func) func = &spawn;
            _Exit(func(data));
            }
        // parent
        jobserver_forked_parent();
        jobserver_acquire_wait(0, 0);
        return pid;
        }
    pid_t jobserver_bg_spawn(char const *args, ...) {
        int n = 0;
        va_list ap;
        va_start(ap, args);
        while (va_arg(ap, char const *)) n ++;
        va_end(ap);
        char const *argv[n + 2];
        va_start(ap, args);
        argv[0] = args;
        char const **next = argv + 1;
        for (char const *a; (a = va_arg(ap, char const *));) {
            *next = a;
            next ++;
            }
        *next = NULL;
        va_end(ap);
        return jobserver_bg(0, argv);
        }
    bool jobserver_exiting() {
        check_init_return(false);
        if (self.slots_held < 0) {
            fatal_usage("too many slots released");
            return false;
            }
        if (self.children < 0) {
            fatal_usage("too many children reaped");
            }
        while (self.children > 0) {
            if (wait(0) == -1) {
                if (errno == EINTR) continue;
                fatal_sysfunc("wait");
                return false;
                }
            if (!jobserver_waited_keep(self.children == 1))
                return false;
            }
        if (!jobserver_acquire_wait(0, 0)) return false;
        while (self.slots_held > 1)
            if (!jobserver_release_keep(1)) return false;
        return true;
        }
// Manual Forking
    void jobserver_forked_parent() {
        self.slots_held --;
        self.children ++;
        }
    void jobserver_forked_child() {
        self.slots_held = 1;
        self.children = 0;
        if (dup2(self.devnull, 0) == -1) {
            _Exit(71);
            }
        self.devnull = 0;
        }
    bool jobserver_waited() {
        self.children --;
        self.slots_held ++;
        return jobserver_release_keep(0);
        }
    bool jobserver_waited_keep(int keep_slots) {
        self.children --;
        self.slots_held ++;
        if (self.slots_held <= keep_slots)
            return true;
        return jobserver_release_keep(keep_slots);
        }
// Low-level Interface
    static void no_op(int _) {};
    bool jobserver_init(jobserver_wait_callback func, void *data) {
        if (self.write_fd) return true;
        // write_fd initialized to 0, but never 0 after successful init

        // first, initialization which can be "shared" with init_or_sync
        // TODO: instead, set a flag to no-op (almost) everthing?
        if (!self.devnull) {
            if ((self.devnull = open("/dev/null", O_RDONLY | O_CLOEXEC)) == -1) {
                fatal_sysfunc("open");
                return false;
                }
            }
        if (!self.child_fd) {
            sigset_t mask;
            if (sigemptyset(&mask) == -1) {
                fatal_sysfunc("sigemptyset");
                return false;
                }
            if (sigaddset(&mask, SIGCHLD) == -1) {
                fatal_sysfunc("sigaddset");
                return false;
                }
            int fd = signalfd(-1, &mask, SFD_CLOEXEC);
            if (fd == -1) {
                fatal_sysfunc("signalfd");
                return false;
                }
            // TODO: inherit sigprocmask?
            if (sigprocmask(SIG_BLOCK, &mask, 0) == -1) {
                fatal_sysfunc("sigprocmask");
                return false;
                }
            self.child_fd = fd;
            }
        if (!self.nonblock_timer) {
            if (timer_create(CLOCK_MONOTONIC, NULL, &self.nonblock_timer) == -1) {
                fatal_sysfunc("timer_create");
                return false;
                }
            else {
                // must have a non-ignored SIGALRM handler to interrupt system calls
                struct sigaction act = {}, oldact;
                act.sa_handler = &no_op;
                if (sigaction(SIGALRM, &act, &oldact) == -1) {
                    fatal_sysfunc("sigaction");
                    return false;
                    }
                if (oldact.sa_handler || oldact.sa_sigaction) {
                    // if previously handled, assume it can handle our signals
                    if (sigaction(SIGALRM, &oldact, NULL) == -1) {
                        fatal_sysfunc("sigaction");
                        return false;
                        }
                    }
                }
            }

        long read_fd, write_fd; {
            char const *env = getenv("JOBSERVER_FDS");
            if (!env) {
                errno = 0;
                return false;
                }
            char const *rest;
            read_fd = strtol(env, (char **) &rest, 10);
            if (*rest != ',') {
                errno = EINVAL;
                fatal_usage("invalid $JOBSERVER_FDS");
                return false;
                }
            write_fd = strtol(rest + 1, (char **) &rest, 10);
            if (*rest != '\0') {
                errno = EINVAL;
                fatal_usage("invalid $JOBSERVER_FDS");
                return false;
                }
            if (read_fd  < 0 || INT_MAX < read_fd
            ||  write_fd < 0 || INT_MAX < write_fd) {
                errno = ERANGE;
                fatal_usage("invalid $JOBSERVER_FDS");
                return false;
                }
            }
        self.read_fd = (int) read_fd;
        // write_fd is sentinel and assigned last
        self.slots_held = 1;  // start with one
        self.write_fd = (int) write_fd;
        return true;
        }
    bool jobserver_set_wait_callback(jobserver_wait_callback func, void *data) {
        self.wait_callback = func;
        self.wait_callback_data = data;
        return true;
        }
    int jobserver_ready_fd() {
        check_init_return(-1);
        return self.read_fd;
        }
    int jobserver_child_fd() {
        check_init_return(-1);
        return self.child_fd;
        }
    static bool acquire() {
        char c;
        ssize_t n;
        while ((n = read(self.read_fd, &c, 1)) == -1) {
            if (errno != EINTR) fatal_sysfunc("read");
            return false;
            }
        if (n == 0) {
            fatal_usage("unexpected EOF");
            return false;
            }
        self.slots_held ++;
        return true;
        }
    static bool try_(bool (*func)()) {
        // should be used only after jobserver_ready_fd is ready for reading, but that doesn't mean reading won't block
        // set an interval timer to interrupt read(2)
        if (timer_settime(self.nonblock_timer, 0, &safe_delay, NULL) == -1) {
            fatal_sysfunc("timer_settime");
            return false;
            }
        bool success = func();
        if (timer_settime(self.nonblock_timer, 0, &no_delay, NULL) == -1) {
            fatal_sysfunc("timer_settime");
            return false;
            }
        return success;
        }
    bool jobserver_try_acquire() {
        // try to own a slot; returns true if acquired or false on error
        check_init_return(false);
        if (self.slots_held > 0) return true;
        return try_(acquire);
        }
    bool jobserver_acquire_wait(jobserver_wait_callback func, void *data) {
        check_init_return(false);
        if (self.slots_held > 0) return true;
        struct pollfd x[2] = {{self.read_fd, POLLIN}, {self.child_fd, POLLIN}};
        while (true) {
            int n = poll(x, 2, -1);
            if (n == -1) {
                if (errno == EINTR) continue;
                fatal_sysfunc("poll");
                return false;
                }
            if (n == 0) continue;
            if (x[1].revents) {
                // child_fd
                pid_t pid;
                int wstatus;
                if ((pid = waitpid(-1, &wstatus, WNOHANG)) == -1) {
                    if (errno == ECHILD) continue;
                    fatal_sysfunc("waitpid");
                    return false;
                    }
                if (!pid) continue;
                (void) jobserver_waited_keep(1);
                if (!func) func = self.wait_callback;
                if (func) {
                    if (!data) data = self.wait_callback_data;
                    if (!func(pid, wstatus, data)) return false;
                    }
                return true;
                }
            if (x[0].revents) {
                if (try_(acquire)) return true;
                }
            }
        }
    bool jobserver_release_keep(int keep_slots) {
        if (keep_slots < 0) fatal_usage("keep_slots < 0");
        check_init_return(false);
        while (self.slots_held > keep_slots) {
            ssize_t n;
            int write_len = self.slots_held - keep_slots;
            if (write_len > 16) write_len = 16;;
            while ((n = write(self.write_fd, "xxxxxxxxxxxxxxxx", write_len)) == -1) {
                if (errno == EINTR) continue;
                fatal_sysfunc("write");
                return false;
                }
            if (n == 0) {
                jobserver_error(70, "write", 0, "unexpected zero-length success");
                return false;
                }
            self.slots_held -= n;
            }
        return true;
        }
