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


extern char const *prog_name;
void fatal(int rc, char const *fmt, ...);
void nonfatal(char const *fmt, ...);

typedef int (*handle_option_func)(char const *name, char const *value, void *data);
int parse_options(int *pargc, char ***pargv, handle_option_func handle_option, void *data);
int main_parse_options(int *pargc, char ***pargv, handle_option_func handle_option, void *data);
int parse_no_options(int *pargc, char ***pargv);
int main_no_options(int *pargc, char ***pargv);

ssize_t write_all(int fd, void const *data, size_t size);

bool to_int(int *dest, char const *s);
