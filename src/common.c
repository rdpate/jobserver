#include "common.h"

char const *prog_name = "<unknown>";

void fatal(int rc, char const *fmt, ...) {
    fprintf(stderr, "%s error: ", prog_name);
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fputc('\n', stderr);
    exit(rc);
    }

void nonfatal(char const *fmt, ...) {
    fprintf(stderr, "%s: ", prog_name);
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fputc('\n', stderr);
    }

static int parse_options_(char **args, char ***rest, handle_option_func handle_option, void *data) {
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
                    if ((rc = handle_option(args[0] + 2, NULL, data))) {
                        break;
                        }
                    }
                else {
                    // long option with value
                    *eq = '\0';
                    if ((rc = handle_option(args[0] + 2, eq + 1, data))) {
                        *eq = '=';
                        break;
                        }
                    *eq = '=';
                    }
                }
            else if (args[0][2] == '\0') {
                // short option without value
                if ((rc = handle_option(args[0] + 1, NULL, data))) {
                    break;
                    }
                }
            else {
                // short option with value
                char name[2] = {args[0][1]};
                if ((rc = handle_option(name, args[0] + 2, data))) {
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
int parse_options(int *pargc, char ***pargv, handle_option_func handle_option, void *data) {
    char **rest;
    int rc = parse_options_(*pargv, &rest, handle_option, data);
    *pargc -= rest - *pargv;
    *pargv = rest;
    return rc;
    }
int main_parse_options(int *pargc, char ***pargv, handle_option_func handle_option, void *data) {
    prog_name = **pargv;
    char const *x = strrchr(prog_name, '/');
    if (x) prog_name = x + 1;
    --*pargc;
    ++*pargv;
    return parse_options(pargc, pargv, handle_option, data);
    }
static int no_options(char const *name, char const *_value, void *_data) {
    fatal(64, "unknown option %s", name);
    return 64;
    }
int parse_no_options(int *pargc, char ***pargv) {
    return parse_options(pargc, pargv, &no_options, NULL);
    }
int main_no_options(int *pargc, char ***pargv) {
    return main_parse_options(pargc, pargv, &no_options, NULL);
    }


bool to_int(int *dest, char const *s) {
    char const *end;
    errno = 0;
    long temp = strtol(s, (char **)&end, 10);
    if (end == s) errno = EINVAL;
    else if (!errno) {
        if (INT_MIN <= temp && temp <= INT_MAX) {
            *dest = temp;
            return true;
            }
        errno = ERANGE;
        }
    return false;
    }
