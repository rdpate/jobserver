exec >&2
fatal() { rc="$1"; shift; printf %s\\n "${0##*/} error: $*" >&2; exit "$rc"; }

src="../src/$1.c"
[ -e "$src" ] || fatal 66 "unknown target: $1"

( exec 0>.gitignore
    flock 0 || fatal 70 "flock failed"
    grep -q -F -x "/$1" .gitignore || printf %s\\n "/$1" >>.gitignore
    )

redo-ifchange "$src"
cc -O2 -o"$3" "$src"
