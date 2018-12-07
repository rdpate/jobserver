#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <unistd.h>

static char const *prog_name = "<unknown>";
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

static char const *var_name = "PIPE_FDS";
bool is_valid_var_name(char const *name) {
    if (!*name) return false;
    if ('0' <= *name && *name <= '9') return false;
    for (; *name; ++name) {
        if ('0' <= *name && *name <= '9') {}
        else if ('A' <= *name && *name <= 'Z') {}
        else if ('a' <= *name && *name <= 'z') {}
        else if (*name == '_') {}
        else return false;
        }
    return true;
    }

static int start_fd = 0;
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

int handle_option(char const *name, char const *value) {
    if (!strcmp(name, "var")) {
        if (!is_valid_var_name(value)) {
            fatal(64, "invalid env variable name: %s", value);
            }
        var_name = value;
        }
    else if (!strcmp(name, "start")) {
        if (!to_int(&start_fd, value) || start_fd < 0) {
            fatal(64, "invalid start fd: %s", value);
            }
        }
    else fatal(64, "unknown option: %s", name);
    return 0;
    }

int parse_options(char **args, char ***rest) {
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

int main(int _, char **argv) {
    { // parse options
        char const *x = strrchr((prog_name = argv[0]), '/');
        if (x) prog_name = x + 1;
        char **rest = 0;
        int rc = parse_options(argv + 1, &rest);
        if (rc) return rc;
        argv = rest;
        }

    int fds[2] = {-1, -1};
    if (pipe(fds)) {
        perror("pipe");
        return 70;
        }

    if (start_fd) {
        int old_read = fds[0];
        if (old_read < start_fd) {
            fds[0] = fcntl(old_read, F_DUPFD, start_fd);
            if (fds[0] == -1) {
                perror("fcntl");
                return 69;
                }
            close(old_read);
            }
        int old_write = fds[1];
        if (old_write < start_fd) {
            fds[1] = fcntl(old_write, F_DUPFD, start_fd);
            if (fds[1] == -1) {
                perror("fcntl");
                return 69;
                }
            close(old_write);
            }
        }

    if (fds[0] > 99999 || fds[1] > 99999) return 70;
    int newlen = strlen(var_name) +  1 + 5 + 1 + 5 + 1;
    //                               '=' %d  ',' %d  '\0'
    char *var = malloc(newlen);
    if (!var) return 70;
    sprintf(var, "%s=%d,%d", var_name, fds[0], fds[1]);
    putenv(var);

    execvp(argv[0], argv);
    perror("exec");
    return 65;
    }
