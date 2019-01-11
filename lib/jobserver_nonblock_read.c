#include "jobserver_nonblock_read.h"
#include "jobserver.h"

#include <errno.h>
#include <signal.h>
#include <time.h>

#include <unistd.h>

#define fatal_sysfunc(func)     jobserver_error(71, func, errno, NULL)

static struct itimerspec const no_delay = {};
static struct itimerspec const safe_delay = {{}, {0, 10 * 1000}};
    // delay before EINTR

static struct {
    timer_t timer_id;
    bool init_done;
    } self;

static void no_op(int _) {}

bool jobserver_nonblock_read_init(void) {
    if (self.init_done) return true;
    if (timer_create(CLOCK_MONOTONIC, NULL, &self.timer_id) == -1) {
        fatal_sysfunc("timer_create");
        return false;
        }
    // timer_create above specifies to generate SIGALRM
    // we just need a non-default, non-ignored handler, to interrupt read with EINTR
    struct sigaction act = {}, oldact;
    act.sa_handler = &no_op;
    if (sigaction(SIGALRM, &act, &oldact) == -1) {
        fatal_sysfunc("sigaction");
        return false;
        }
    if (oldact.sa_handler || oldact.sa_sigaction) {
        // if previously set, assume it can also handle our signals, and put it back
        if (sigaction(SIGALRM, &oldact, NULL) == -1) {
            fatal_sysfunc("sigaction");
            return false;
            }
        }
    self.init_done = true;
    return true;
    }

ssize_t jobserver_nonblock_read(int fd, void *buffer, size_t length) {
    if (timer_settime(self.timer_id, 0, &safe_delay, NULL) == -1) {
        fatal_sysfunc("timer_settime");
        return false;
        }
    ssize_t result = read(fd, buffer, length);
    if (timer_settime(self.timer_id, 0, &no_delay, NULL) == -1) {
        if (errno != EINTR) fatal_sysfunc("timer_settime");
        }
    return result;
    }
