// headers
    #include "block_readwrite.h"
    #include "unreachable.h"
    #include <errno.h>
    #include <stdbool.h>

    #include <poll.h>
    #include <unistd.h>
ssize_t block_read(int fd, void *buffer, size_t length) {
    while (true) {
        int events = block_poll_fd(fd, POLLIN);
        if (events == -1) {
            if (errno == EAGAIN) continue;
            if (errno == EINTR) continue;
            return -1;
            }
        if (events & POLLNVAL) {
            errno = EBADF;
            return -1;
            }
        if (events & POLLERR) {
            errno = EIO;
            return -1;
            }
        if (events & POLLHUP) {
            // must still drain readable data
            // and then let read return 0
            }
        else if (!(events & POLLIN)) { UNREACHABLE }
        ssize_t actual_len = read(fd, buffer, length);
        if (actual_len == -1) {
            if (errno == EAGAIN) continue;
            if (errno == EWOULDBLOCK) continue;
            if (errno == EINTR) continue;
            return -1;
            }
        return actual_len;
        }
    }
ssize_t block_write(int fd, void const *data, size_t length) {
    size_t remaining = length;
    char const *next = (char const *) data;
    while (remaining) {
        ssize_t n = block_write_some(fd, next, remaining);
        if (n == -1) {
            if (remaining < length) return length - remaining;
            return -1;
            }
        if (n == 0) return length - remaining;
        remaining -= n;
        next += n;
        }
    return length;
    }
ssize_t block_write_some(int fd, void const *data, size_t length) {
    while (true) {
        int events = block_poll_fd(fd, POLLOUT);
        if (events == -1) {
            if (errno == EAGAIN) continue;
            if (errno == EINTR) continue;
            return -1;
            }
        if (events & POLLNVAL) {
            errno = EBADF;
            return -1;
            }
        if (events & (POLLERR | POLLHUP)) {
            // let write report the error
            }
        else if (!(events & POLLOUT)) { UNREACHABLE }
        ssize_t actual_len = write(fd, data, length);
        if (actual_len == -1) {
            if (errno == EAGAIN) continue;
            if (errno == EWOULDBLOCK) continue;
            if (errno == EINTR) continue;
            return -1;
            }
        return actual_len;
        }
    }
