Optionally Parallel
====

If xjob is on $PATH, then use it, starting if needed.  Otherwise run background jobs synchronously:

    if which xjob >/dev/null 2>&1; then
        xjob started || exec xjob init "$0" "$@"
        bg_job() {
            (trap 'xjob release' exit; command "$@" </dev/null) &
            xjob acquire
            }
    else
        # synchronous
        bg_job() { "$@" </dev/null; }
        # redirect stdin to avoid unintentional differences
        fi

Note: this will "exec $0", which could change behavior compared to "sh -flags file".
