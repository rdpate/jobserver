#ifndef INCLUDE_GUARD_a56479732101de78130031a57772e46a
#define INCLUDE_GUARD_a56479732101de78130031a57772e46a

#include <stdbool.h>

void init_monitor(double target_load, int leak_warning);
bool slot_change(int read_fd, int write_fd);
    // - perform slot adjustment, if needed
    // return: whether adjustment was required

#endif
