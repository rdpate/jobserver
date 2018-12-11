fail() {  # RC MSG [PREMSG..]
    rc="$1"
    msg="$2"
    shift 2
    for x; do printf %s\\n "$x"; done
    printf %s\\n "fail $rc $msg"
    exit "$rc"
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

assert_success() {  # RC CMD..
    local rc output
    rc="$1"
    shift
    output="$("$@" 2>&1)" || fail "$rc" "expected command success: $*" "$output"
    }
assert_fails() {  # RC CMD..
    local rc output
    rc="$1"
    shift
    output="$("$@" 2>&1)" && fail "$rc" "expected command failure: $*" "$output" || true
    }

assert_empty() {  # RC NAME; test $NAME is empty
    set -- "$@" "$(eval printf %s \"\${$2-}\")"
    [ -z "$3" ] || fail "$1" "expected empty $2 value" "$2=$3"
    }
assert_nonempty() {  # RC NAME; test $NAME is non-empty
    set -- "$@" "$(eval printf %s \"\${$2-}\")"
    [ -n "$3" ] || fail "$1" "expected non-empty $2 value"
    }

assert_equals() {  # RC EXPECTED ACTUAL
    [ x"$2" = x"$3" ] || fail "$1" '' "expected: $(squote "$3")" "  actual: $(squote "$2")"
    }
