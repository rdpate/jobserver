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
    if (**rest != ' ' && **rest != '\0') {
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
    }
static void load_env_fds() {
    bool found = false;
    char *env, *rest;
    if ((env = getenv("JOBSERVER_FDS")))
        if (extract_fds(env, &rest, env_fds) && *rest == '\0')
            found = true;
    if (!found && (env = getenv("MAKEFLAGS"))) {
        while (*env) {
            while (*env == ' ') ++env;
            if (strncmp(env, "--jobserver-fds=", 16) == 0) {
                env += 16; // 1234567890ABCDEF
                if (extract_fds(env, &rest, env_fds))
                    if (*rest == '\0' || *rest == ' ') {
                        found = true;
                        env = rest;
                        continue;
                        }
                env = strchr(env, ' ');
                if (!env) break;
                }
            if (strncmp(env, "--jobserver-auth=", 17) == 0) {
                env += 17; // 1234567890ABCDEFG
                if (extract_fds(env, &rest, env_fds))
                    if (*rest == '\0' || *rest == ' ') {
                        found = true;
                        env = rest;
                        continue;
                        }
                env = strchr(env, ' ');
                if (!env) break;
                }
            while (*env && *env != ' ') ++env;
            }
        }
    if (!found) fatal(64, "jobserver not running!");
    env_fds_ready = true;
    }
static int get_read_fd() {
    if (!env_fds_ready) load_env_fds();
    return env_fds[0];
    }
static int get_write_fd() {
    if (!env_fds_ready) load_env_fds();
    return env_fds[1];
    }
