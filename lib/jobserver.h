#include <stdbool.h>
#include <poll.h>
#include <sys/types.h>

// Requirements
    void jobserver_error(int exitcode, char const *func, int errnum, char const *msg);
        // - report an error
        // exitcode:
        //  - exit code if fatal or 0 for log output
        //  - exit(3) allowed but not required
        //  - if 0, errnum is ignored
        // func: relevant function or NULL
        // errnum: errno if applicable or 0 otherwise
        // msg: brief description or NULL
        // - see jobserver_fatal.c for sample definition
// High-level Interface
    bool jobserver_init_or_exec(char **self_args);
        // - init or exec "jobserver init --" plus self_args
        // return: whether successful (maybe not at all)
    bool jobserver_init_or_sync();
        // - init or create synchronous jobserver (1 slot)
        // error: false
        // return: whether successful
    pid_t jobserver_bg(int (*func)(void *data), void *data);
        // - if func is NULL, use function which calls execvp(*(char**)data, data) and returns 71 on error
        // - fork new "background" process
        //  - child runs _Exit(func(data))
        //  - not background as in shell job control
        // - stdin redirected from /dev/null
        // - ensures parent holds a slot when exits, which might involve waiting for a child
        // error: -1
        // return: child PID
    pid_t jobserver_bg_spawn(char const *cmd, char const *args, ...);
        // - collect cmd and args into argv array
        // - do not repeat cmd in args
        // delegate: jobserver_bg(0, argv)
    pid_t jobserver_bg_shell(char const *script, char const *args, ...);
        // - collect script and args into argv array
        // delegate: jobserver_bg_spawn("/bin/sh", "-uec", script, "[-c]", args);
    bool jobserver_exiting();
        // - wait for children
        // - release/aqcquire to hold exactly 1 slot
        // error: false
        // return: success
        // note: might have inherited children, must track PIDs to know
// Manual Forking
    void jobserver_forked_parent();
        // - use in parent after forking
    void jobserver_forked_child();
        // - use in child after forking
        // - also redirects stdin from /dev/null
    bool jobserver_waited();
        // call from parent after collecting a child; return false on error
        // releases child's slot
    bool jobserver_waited_keep(int keep_slots);
        // call from parent after collecting a child; return false on error
        // transfers ownership of child's slot to parent
        // releases child's slot if now hold more than keep_slots
        // only use keep_slots if will immediately be starting that many jobs
// Low-level Interface
    bool jobserver_init();
        // - initialize jobserver
        // - no-op if already succeeded
        // error: false, see errno
        //  - if not running, errno = 0 and jobserver_fatal not called
        // return: success
    typedef bool (*jobserver_wait_callback)(pid_t pid, int wstatus, void *data);
    bool jobserver_set_wait_callback(jobserver_wait_callback func, void *data);
        // - save func as default acquire_wait callback
        // - save data as default data for func
        // error: false
        // return: success
    int jobserver_ready_fd();
        // - read fd is used to indicate job slots are ready
        // error: -1
        // return: fd
    int jobserver_child_fd();
        // - signalfd which only receives SIGCHLD
    bool jobserver_try_acquire();
        // acquire a slot without blocking
        // error: false
        // return: whether acquired
        // note: may generate SIGALRM
    bool jobserver_acquire_wait(jobserver_wait_callback func, void *data);
        // - ensure holding a slot
        // - if reaped a child
        //  - call jobserver_waited_keep(1)
        //  - use callback or default callback
        //      - data argument is data or default data
        // error: false
        // return: propagate callback, or true if no callback
    bool jobserver_release_keep(int keep_slots);
        // - release held slots, if more than keep_slots
        // error: false
        // return: success
