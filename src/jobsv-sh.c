#include "common.h"

int main(int argc, char **argv) {
    prog_name = "jobsv-sh";
    --argc;
    ++argv;
    int rc = parse_no_options(&argc, &argv);
    if (rc) return rc;
    if (argc == 0) fatal(64, "missing subcommand argument");
#define SUB(NAME) do {                                                  \
    int main_##NAME(int, char **);                                      \
    if (strcmp(argv[0], #NAME) == 0) {                                  \
        prog_name = "jobsv-sh " #NAME;                                  \
        return main_##NAME(argc - 1, argv + 1);                         \
        }                                                               \
    } while (0);
    SUB(started)
    SUB(release)
    SUB(acquire)
    fatal(64, "unknown subcommand %s", argv[0]);
    }
