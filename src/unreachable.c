#include "unreachable.h"
#include <stdio.h>
void unreachable(char const *file, int line, char const *func) {
    fprintf(stderr, "%s:%d (%s) should be unreachable!\n", file, line, func);
    }
