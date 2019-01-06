#include "monitor.h"
#include "common.h"
#include "block_readwrite.h"
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>

static struct {
        double target_begin, target_load, target_end;
        int added, leak_warning;
    } self;
void init_monitor(double target_load, int leak_warning) {
    // target load is actually a range
        self.target_begin  = target_load - 0.5;
        self.target_load   = target_load;
        self.target_end    = target_load + 0.5;
    self.added = 0;
    self.leak_warning = leak_warning;
    }
static bool slots_are_waiting(int read_fd) {
    struct pollfd x = {read_fd, POLLIN};
    int rc = poll(&x, 1, 0);
    if (rc == -1) {
        perror("poll");
        return false;
        }
    if (rc == 0) return false;
    return (x.revents & POLLIN);
    }
static int slot_adjustment(int read_fd) {
    double current = self.target_load;
    getloadavg(&current, 1);
    if (self.target_end < current) {
        // need to reduce load
        return self.added ? -1 : 0;
        }
    if (current < self.target_begin) {
        // either children are being selfish...
        if (!slots_are_waiting(read_fd)) {
            // I'm givin' her all she's got, Captain!
            return 1;
            }
        // ...or currently lacking work
        return self.added ? -1 : 0;
        }
    return 0;  // currently within Goldilocks Zone
    }
static void add_slot(int write_fd) {
    ssize_t x = block_write(write_fd, "x", 1);
    if (x == -1) {
        perror("write");
        nonfatal("lost slot due to write error");
        return;
        }
    if (x == 1) {
        self.added ++;
        if (self.added == self.leak_warning) {
            nonfatal("probable job slot leak detected!");
            self.added /= 2;
            }
        return;
        }
    UNREACHABLE
    }
static void remove_slot(int read_fd) {
    char x;
    int rc = block_read(read_fd, &x, 1);
    if (rc == 1) {
        self.added --;
        return;
        }
    if (rc == 0) fatal(70, "unexpected EOF in remove_slot");
    perror("read");
    }
bool slot_change(int read_fd, int write_fd) {
    int adj = slot_adjustment(read_fd);
    if (!adj) return false;
    if (adj < 0) remove_slot(read_fd);
    else add_slot(write_fd);
    return true;
    }

