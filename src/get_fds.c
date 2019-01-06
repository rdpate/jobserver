#include "get_fds.h"
#include "common.h"


static int env_fds[2];
static bool env_fds_ready;
static bool extract_fds(char *s, char **rest, int *dest) {
    // extract "N,N" from s, write to dest[0] and dest[1], return whether successful
    // if successful, *rest is space or null
    // if unsuccessful, *rest is next unread character
    long first = strtol(s, rest, 10);
    if (**rest != ',') {
        errno = EINVAL;
        return false;
        }
    long second = strtol(*rest + 1, rest, 10);
    if (**rest != '\0') {
        errno = EINVAL;
        return false;
        }
    if (first  < 0 || INT_MAX < first
    ||  second < 0 || INT_MAX < second) {
        errno = ERANGE;
        return false;
        }
    dest[0] = (int) first;
    dest[1] = (int) second;
    return true;
    }
bool load_env_fds(void) {
    if (env_fds_ready) return true;
    char *env = getenv("JOBSERVER_FDS");
    if (!env) return false;
    char *rest;
    if (extract_fds(env, &rest, env_fds) && *rest == '\0')
        env_fds_ready = true;
    return env_fds_ready;
    }

int get_read_fd(void) {
    if (!load_env_fds()) fatal(69, "jobserver not running!");
    return env_fds[0];
    }
int get_write_fd(void) {
    if (!load_env_fds()) fatal(69, "jobserver not running!");
    return env_fds[1];
    }
