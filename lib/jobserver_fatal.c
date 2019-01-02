#include "jobserver.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void jobserver_error(int exitcode, char const *func, int errnum, char const *msg) {
    if (!exitcode) {
        if (!msg) msg = strerror(errnum);
        if (!msg) {
            if (func) fprintf(stderr, "%s\n", func);
            // else no exitcode, no func, no errnum, no msg
            }
        else if (func) fprintf(stderr, "(%s) %s\n", func, msg);
        else fprintf(stderr, "%s\n", msg);
        return;
        }

    if (errnum <= 0) {
        if (!msg) msg = "unknown";
        if (!func) fprintf(stderr, "fatal: %s\n", msg);
        else fprintf(stderr, "fatal: %s: %s\n", func, msg);
        }
    else {
        if (!msg) msg = strerror(errnum);
        if (!func) fprintf(stderr, "fatal: %s [errno %d]\n", msg, errnum);
        else fprintf(stderr, "fatal: %s: %s [errno %d]\n", func, msg, errnum);
        }
    exit(exitcode);
    }
