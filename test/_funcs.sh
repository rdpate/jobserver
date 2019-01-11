failed() {  # MSG [PREMSG..]
    msg="$1"
    shift
    for x; do printf %s\\n "$x"; done
    printf %s\\n "failed $msg"
    exit 1
    }
squote() {
    local x
    while true; do
        x="$(printf %s "$1" | tr -d -c \\055_0-9A-Za-z)"
        if [ -n "$1" -a x"$x" = x"$1" ]; then
            printf %s "$1"
        else
            printf \'%s\' "$(printf %s "$1" | sed -r s/\'/\'\\\\\'\'/g)"
            fi
        shift
        case $# in
            0) break ;;
            *) printf ' ' ;;
            esac
        done
    }

assert_success() {  # CMD..
    output="$("$@" 2>&1)" ||
    failed "expected command success: $(squote "$@")" "${output:-(no output)}"
    }
assert_fails() {  # CMD..
    output="$("$@" 2>&1)" &&
    failed "expected command failure: $(squote "$@")" "${output:-(no output)}" || true
    }
assert_exitcode() {  # CODE CMD..
    ec="$1"
    shift
    if [ 0 = "$ec" ]; then
        assert_success "$@"
    else
        output="$("$@" 2>&1)" && failed "expected command failure: $(squote "$@")" "$output" || rc=$?
        if [ "$rc" != "$ec" ]; then
            failed "expected exit code $ec, got $rc: $(squote "$@")" "${output:-(no output)}"
            fi
        fi
    }

assert_empty() {  # NAME; test $NAME is empty
    set -- "$1" "$(eval printf %s \"\${$1-}\")"
    [ -z "$2" ] || failed "empty $1" "expected empty $1 variable" "$1=$2"
    }
assert_nonempty() {  # NAME; test $NAME is non-empty
    set -- "$1" "$(eval printf %s \"\${$1-}\")"
    [ -n "$2" ] || failed "nonempty $1" "expected non-empty $1 variable"
    }

assert_equals() {  # EXPECTED ACTUAL [MSG..]
    if [ x"$1"x != x"$2"x ]; then
        e="expected: $(squote "$1")"
        a="  actual: $(squote "$2")"
        shift 2
        set -- "$@" "$e" "$a"
        failed equals "$@"
        fi
    }
assert_different() {  # EXPECTED ACTUAL [MSG..]
    if [ x"$1"x = x"$2"x ]; then
        e='expected: (anything else)'
        a="  actual: $(squote "$2")"
        shift 2
        set -- "$@" "$e" "$a"
        failed different "$@"
        fi
    }
