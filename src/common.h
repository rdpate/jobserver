#ifndef INCLUDE_GUARD_bf8071438adf386868d5d41cdb317ddb
#define INCLUDE_GUARD_bf8071438adf386868d5d41cdb317ddb

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
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "block_readwrite.h"
#include "unreachable.h"


#define SYS_ERR_FATAL(pre,func,params,err_value,exitcode) do {      \
    if ((pre func params) == (err_value)) {                         \
        fatal(exitcode, #func ": %s", strerror(errno));             \
        }                                                           \
    } while (0)
#define SYS(func,params) SYS_ERR_FATAL(,func,params,-1,71)
#define SYSV(var,func,params) SYS_ERR_FATAL(var =,func,params,-1,71)

extern char const *prog_name;
void fatal(int rc, char const *fmt, ...);
void nonfatal(char const *fmt, ...);

typedef int (*handle_option_func)(char const *name, char const *value, void *data);
int parse_options(int *pargc, char ***pargv, handle_option_func handle_option, void *data);
int main_parse_options(int *pargc, char ***pargv, handle_option_func handle_option, void *data);
int parse_no_options(int *pargc, char ***pargv);
int main_no_options(int *pargc, char ***pargv);

bool to_int(int *dest, char const *s);

#endif
