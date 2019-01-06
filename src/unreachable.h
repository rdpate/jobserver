#ifndef INCLUDE_GUARD_AB33CDC6636548DA861415840CBA3AC3
#define INCLUDE_GUARD_AB33CDC6636548DA861415840CBA3AC3

#include <stdlib.h>

#define UNREACHABLE do {                                        \
    unreachable(__FILE__, __LINE__, __func__);                  \
    exit(70);                                                   \
    } while (0);

void unreachable(char const *file, int line, char const *func);

#endif
