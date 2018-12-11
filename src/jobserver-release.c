#include "common.inc.c"
#include "get_fds.inc.c"

static bool keep_stdin = false;
static int handle_option(char const *name, char const *value) {
    if (!strcmp(name, "keep-stdin")) {
        if (value) fatal(64, "unexpected value for option keep-stdin");
        keep_stdin = true;
        }
    else {
        nonfatal("unknown option: %s (ignored)", name);
        return 64;
        }
    return 0;
    }

// note: main and main2 must (attempt to) release a slot before exiting
static int main2(char **argv) {
    if (!keep_stdin) {
        int devnull = open("/dev/null", O_RDONLY);
        while (dup2(devnull, 0) == -1) {
            switch (errno) {
            case EBUSY:
            case EINTR:
                break;
            default:
                perror("dup2");
                nonfatal("dup2 failed");
                return 71;
            }
        }
        close(devnull);
        }
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        nonfatal("fork failed");
        return 71;
        }
    if (pid == 0) {
        // child
        execvp(argv[0], argv);
        perror("exec");
        exit(65);
        }
    // parent
    int rc, status;
    while (true) {
        pid_t rc = waitpid(pid, &status, 0);
        if (rc == -1) {
            if (errno != EINTR) {
                perror("waitpid");
                fatal(71, "waitpid failed");
                }
            }
        else if (rc == pid) {
            if (WIFEXITED(status)) {
                rc = WEXITSTATUS(status);
                break;
                }
            else if (WIFSIGNALED(status)) {
                rc = WTERMSIG(status) + 128;
                break;
                }
            }
        }
    return rc;
    }
int main(int argc, char **argv) {
    int rc;
    { // parse options
        char const *x = strrchr((prog_name = argv[0]), '/');
        if (x) prog_name = x + 1;
        char **rest = 0;
        rc = parse_options(argv + 1, &rest);
        if (rc == 0) {
            argc -= rest - argv;
            argv = rest;
            }
        }
    int write_fd = get_write_fd();
    if (rc == 0) {
        if (!argc) {
            nonfatal("missing CMD argument");
            rc = 64;
            }
        else rc = main2(argv);
        }
    int _ = write(write_fd, "x", 1);
    return rc;
    }
