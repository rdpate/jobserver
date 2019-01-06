if which jobsv jobsv-sh >/dev/null 2>&1; then
    jobsv_or_exec() {
        # usage: jobsv_or_exec "$0" "$@"
        # note: exec $0 may be different interpreter or shell flags than were used
        [ -n "${1-}" ] || {
            printf %s\\n 'jobsv_started_or_exec: missing args' >&2 || true
            exit 70
            }
        jobsv-sh started || {
            rc=$?
            case "$rc" in
                69) ;;
                *) exit "$rc" ;;
                esac
            exec jobsv "$@"
            }
        }
    jobsv_bg() {
        # usage: jobsv_bg CMD..
        jobsv-sh release "$@" </dev/null &
        jobsv-sh acquire
        }
    jobsv_sleep() {
        jobsv-sh release true
        }
    jobsv_woke() {
        jobsv-sh acquire &
        }
    jobsv_sleeping() {
        local rc
        jobsv_sleep
        "$@" || {
            rc=$?
            jobsv-sh acquire &
            return "$rc"
            }
        jobsv-sh acquire &
        }
else
    jobsv_or_exec() { true; }
    jobsv_bg() { "$@" </dev/null; }
    jobsv_sleep() { true; }
    jobsv_woke() { true; }
    jobsv_sleeping() { "$@"; }
    fi
trap wait exit
