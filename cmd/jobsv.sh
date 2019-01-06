#!/bin/which
# file is executable only to be found by which
# if you're reading this, you might want to copy the file into your script
if which jobsv jobsv-sh >/dev/null 2>&1; then
    jobsv_missing=false
    jobsv_or_exec() {
        [ -n "${1:-}" ] || {
            printf %s\\n "${0##*/} error: jobsv_or_exec missing args" >&2 || true
            exit 70
            }
        jobsv-sh started || {
            rc=$?
            case "$rc" in
                69) ;;
                *) exit "$rc" ;;
                esac
            exec jobsv -- "$@"
            }
        }
    jobsv_bg() {
        jobsv-sh release -- "$@" &
        jobsv-sh acquire
        }
    jobsv_bg_stdin() {
        jobsv-sh release --keep-stdin -- "$@" &
        jobsv-sh acquire
        }
    # jobsv_sleep() { jobsv-sh release true; }
    # jobsv_woke() { jobsv-sh acquire & }
    jobsv_sleeping() {
        jobsv-sh release true
        "$@" || {
            rc=$?
            jobsv-sh acquire &
            return $rc
            }
        jobsv-sh acquire &
        }
    jobsv_exiting() { wait; }
    trap wait exit
else
    jobsv_missing=true
    jobsv_or_exec() {
        [ -n "${1:-}" ] || {
            printf %s\\n "${0##*/} error: jobsv_or_exec missing args" >&2 || true
            exit 70
            }
        printf %s\\n "${0##*/}: jobserver synchronous fallback" >&2 || true
        }
    jobsv_bg() { "$@" </dev/null; }
    jobsv_bg_stdin() { "$@"; }
    # jobsv_sleep() { true; }
    # jobsv_woke() { true; }
    jobsv_sleeping() { "$@"; }
    jobsv_exiting() { true; }
    fi
