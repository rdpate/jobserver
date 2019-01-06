#ifndef INCLUDE_GUARD_4db540e8e178c59cd655040c018c3a20
#define INCLUDE_GUARD_4db540e8e178c59cd655040c018c3a20

#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>

bool load_env_fds(void);
int get_read_fd(void);
int get_write_fd(void);

#endif
