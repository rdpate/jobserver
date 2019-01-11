#ifndef INCLUDE_GUARD_04a541ab64c36dc9c52ba820ce0b23a8
#define INCLUDE_GUARD_04a541ab64c36dc9c52ba820ce0b23a8

// There are various possibilities to implement nonblock_read, which can be selected by replacing the implementation of these functions.

#include <stdbool.h>
#include <sys/types.h>

bool jobserver_nonblock_read_init(void);
    // - any needed initialization
    // error: false
    // return: whether successful
ssize_t jobserver_nonblock_read(int fd, void *buffer, size_t length);
    // - read at most length bytes from fd into buffer
    // - non-blocking
    // error: -1
    // return: number of bytes read

#endif
