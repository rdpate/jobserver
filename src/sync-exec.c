#include "common.h"

int main_sync_exec(int argc, char **argv) {
    int rc = parse_no_options(&argc, &argv);
    if (rc) return rc;
    if (unsetenv("JOBSERVER_FDS") == -1) {
        perror("unsetenv");
        fatal(71, "unsetenv failed");
        }
    execvp(argv[0], argv);
    perror("exec");
    fatal(71, "exec failed");
    return 71;
    }
