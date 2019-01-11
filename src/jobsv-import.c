#include "common.h"

static struct {
    bool no_makeflags;
    } self;
static int const start_fd = 100;
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
    if (self.no_makeflags) return;
    int make_read_fd, make_write_fd;
    SYSV( make_read_fd,  fcntl,(read_fd,  F_DUPFD, start_fd) );
    SYSV( make_write_fd, fcntl,(write_fd, F_DUPFD, start_fd) );
    set_makeflags(make_read_fd, make_write_fd);
    }
static int handle_option(char const *name, char const *value, void *data) {
    if (strcmp(name, "no-makeflags") == 0) {
        if (value) fatal(64, "unexpected value for option %s", name);
        self.no_makeflags = true;
        }
    else fatal(64, "unknown option %s", name);
    return 0;
    }
int main(int argc, char **argv) {
    int rc = main_parse_options(&argc, &argv, &handle_option, NULL);
    if (rc) return rc;
    if (!*argv) fatal(64, "missing FILE CMD arguments");
    if (!argv[1]) fatal(64, "missing CMD argument");

    int fd;
    { // setup pipe
        fd = open(*argv, O_RDWR);
        if (fd == -1) fatal(65, "%s", strerror(errno));
        int const start_fd = 100;
        if (fd < start_fd) {
            int old_fd = fd;
            SYSV( fd, fcntl,(old_fd, F_DUPFD, start_fd) );
            close(old_fd);
            }
        argv ++;
        }
    if (fd > 99999) return 70;
    char buffer[5+1+5+1];
    sprintf(buffer, "%d,%d", fd, fd);
    SYS( setenv,("JOBSERVER_FDS", buffer, true) );
    if (!self.no_makeflags) do_makeflags(fd, fd);
    execvp(argv[0], argv);
    fatal(65, "exec: %s", strerror(errno));
    }
