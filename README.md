Jobserver
=========

Implement a jobserver compatible with [redo](https://redo.rtfd.io/) and GNU Make.


## Getting Started

Run task/build, then task/test.

Redo is used, either install redo or read task/build and default.do to execute those steps manually.  To install, symlink cmd/jobserver into $PATH (or cp cmd/\*jobserver\* dir\_in\_PATH/).

See the examples provided in doc, of which doc/xlines is roughly equivalent to "xargs -d\\\\n -n1 -PN", where N is the number of slots (see Defaults).

Run your entire shell session under Jobserver (exec jobserver init "$SHELL") or customize particular commands (see doc/make-shim).  For example, run all tmux panes (everything started within a tmux session) under a shared Jobserver:

    # if $PATH includes below location before system directories
    $ file=~/.local/binsys/tmux
    # don't pick up our override if already exists!
    $ tmux="$(which -a tmux | grep -v -F -x "$file" | head -n1)"
    $ echo $tmux
    $ cat <<END >"$file"
    #!/bin/sh -ue
    if [ \$# = 0 ] || [ new = "$1" -o new-session = "$1" ]; then
        exec jobserver init $tmux "$@"
    fi
    exec $tmux "\$@"
    END
    # "jobserver init" default is to reuse a pool, so run it unconditionally

Write a shell script to run under Jobserver, starting if needed:

    if which jobserver >/dev/null 2>&1; then
        jobserver started || exec jobserver init "$0" "$@"
        # note: could change behavior compared to "sh -flags $0"
        bg_job() {
            jobserver release "$@" &
            # "current" slot has passed to bg command, must acquire to continue
            jobserver acquire
        }
    else
        # fallback to synchronous
        # redirect stdin to avoid unintentional differences
        bg_job() { "$@" </dev/null; }
    fi

    # Don't forget to wait for those background jobs!
    trap wait exit

## Defaults

By default, this Jobserver attempts to be smart:

* active running tasks ("slots") defaults to number of (available) processors
* slots is the target system load
* if load average stays lower than slots and no slots are available, increase slots
* if load average stays higher than slots, remove an added slot (without going below the initial number)
* load average is checked every minute, and the slot number is compared against the minute average
