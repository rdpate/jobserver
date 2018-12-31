#include "common.h"

int main(int argc, char **argv) {
    prog_name = "jobserver";
    --argc;
    ++argv;
    int rc = parse_no_options(&argc, &argv);
    if (rc) return rc;
    if (argc == 0) fatal(64, "missing subcommand argument");
#define SUB2(NAME,FUNC) \
    int main_##FUNC(int, char **); \
    if (strcmp(argv[0], #NAME) == 0) { \
        prog_name = "jobserver " #NAME; \
        return main_##FUNC(argc - 1, argv + 1); \
        }
#define SUB(NAME) SUB2(NAME,NAME)
    SUB(init)
    SUB(started)
    SUB(release)
    SUB(acquire)
    SUB2(sleep-start,sleep_start)
    SUB2(sleep-end,sleep_end)
    SUB2(sync-exec,sync_exec)
    fatal(64, "unknown subcommand %s", argv[0]);
    }
