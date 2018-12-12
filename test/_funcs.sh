fail() {  # MSG [PREMSG..]
    msg="$1"
    shift
    for x; do printf %s\\n "$x"; done
    printf %s\\n "fail $msg"
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
    output="$("$@" 2>&1)" || fail "expected command success: $*" "$output"
    }
assert_fails() {  # CMD..
    output="$("$@" 2>&1)" && fail "expected command failure: $*" "$output" || true
    }

assert_empty() {  # NAME; test $NAME is empty
    set -- "$1" "$(eval printf %s \"\${$1-}\")"
    [ -z "$2" ] || fail "expected empty $1 value" "$1=$2"
    }
assert_nonempty() {  # NAME; test $NAME is non-empty
    set -- "$1" "$(eval printf %s \"\${$1-}\")"
    [ -n "$2" ] || fail "$1" "expected non-empty $1 value"
    }

assert_equals() {  # EXPECTED ACTUAL
    [ x"$1" = x"$2" ] || fail equals "expected: $(squote "$1")" "  actual: $(squote "$2")"
    }
