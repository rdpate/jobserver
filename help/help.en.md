Jobserver
====

Jobserver parallelizes jobs to better utilize processing power, and does so better than common alternatives:

* reacts to system load
* can be long-lived, spanning any size process tree
    * including multiple login sessions
* re-creates leaked job slots

Compatible with [redo](https://redo.rtfd.io/) and GNU Make.


Getting Started
----

Run task/build, then task/test.

Redo is used, either install redo or read task/build and default.do to execute those steps manually.  To install, symlink or copy executables from cmd into \$PATH.

Run your entire shell session under Jobserver or write a shell script to run under Jobserver, starting if needed:

    if jobsv_sh="$(which jobsv.sh 2>/dev/null)"; then
        . "$jobsv_sh"
        jobsv_or_exec "$0" "$@"
        # note: could change behavior compared to "sh -flags $0"
    else
        # fallback to synchronous
        # redirect stdin to avoid unintentional differences
        jobsv_bg() { "$@" </dev/null; }
    fi
    # ...
    jobsv_bg echo "some command run in bg"

An example of the above is doc/xlines, which is roughly equivalent to "xargs -d\\\\n -n1 -PN", except:

* jobsv automatically determines and monitors the number of slots
* job slots are shared with children


Defaults
----

By default, Jobserver attempts to be smart:

* active running tasks ("slots") defaults to number of (available) processors
* monitors the pool's total slots in comparsion with the target system load
* load average is checked every minute, and the slot number is compared against the minute average
* if load average stays lower than target and no slots are available, increase slots
    * indicates either sleeping while holding or a leak
* if load average stays higher than slots, decrease slots
    * only removes slots added by the monitor

Instead of the above, if you want behavior similar to "-jN" in many programs, use the option --fixed=N.
