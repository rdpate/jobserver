#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

static char const *prog_name = "<unknown>";
static void fatal(int rc, char const *fmt, ...) {
    fprintf(stderr, "%s error: ", prog_name);
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fputc('\n', stderr);
    exit(rc);
    }
static void nonfatal(char const *fmt, ...) {
    fprintf(stderr, "%s: ", prog_name);
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fputc('\n', stderr);
    }

static int handle_option(char const *name, char const *value);
static int parse_options(char **args, char ***rest) {
    // modifies argument for long options with values, though does reverse the modification afterwards
    // returns 0 or the first non-zero return from handle_option
    // if rest is non-null, *rest is the first unprocessed argument, which will either be the first non-option argument or the argument for which handle_option returned non-zero
    int rc = 0;
    for (; args[0]; ++args) {
        if (!strcmp(args[0], "--")) {
            args += 1;
            break;
            }
        else if (args[0][0] == '-' && args[0][1] != '\0') {
            if (args[0][1] == '-') {
                // long option
                char *eq = strchr(args[0], '=');
                if (!eq) {
                    // long option without value
                    if ((rc = handle_option(args[0] + 2, NULL))) {
                        break;
                        }
                    }
                else {
                    // long option with value
                    *eq = '\0';
                    if ((rc = handle_option(args[0] + 2, eq + 1))) {
                        *eq = '=';
                        break;
                        }
                    *eq = '=';
                    }
                }
            else if (args[0][2] == '\0') {
                // short option without value
                if ((rc = handle_option(args[0] + 1, NULL))) {
                    break;
                    }
                }
            else {
                // short option with value
                char name[2] = {args[0][1]};
                if ((rc = handle_option(name, args[0] + 2))) {
                    break;
                    }
                }
            }
        else break;
    }
    if (rest) {
        *rest = args;
        }
    return rc;
    }
