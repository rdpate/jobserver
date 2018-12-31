set -Cue
exec >&2
fatal() { rc="$1"; shift; printf %s\\n "${0##*/} error: $*" >&2 || true; exit "$rc"; }

target="$1"
output="$3"
link="../src/$target.link"
[ -e "$link" ] || fatal 66 "unknown target $1"
redo-ifchange "$link"
exec <"$link"
set --
while read -r line; do
    case "$line" in
        '#'*|'') continue ;;
        esac
    set -- "$@" "../src/$line"
    done
set -- "../src/$target.o" "$@"
redo-ifchange "$@"
cc -o"$output" "$@"
