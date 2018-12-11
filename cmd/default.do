exec >&2
fatal() { rc="$1"; shift; printf %s\\n "${0##*/} error: $*" >&2; exit "$rc"; }

src="../src/$1.c"
[ -e "$src" ] || fatal 66 "unknown target: $1"
dep="../src/.$1.dep"

redo-ifchange "$src"
cc -MD -MF "$dep" -O2 -o"$3" "$src"
sed -i -r 's/^[^ ]+: //; s/ \\$//; s/^ +| +$//g; s/ +/\n/g; /^$/d' "$dep"
xargs redo-ifchange <"$dep"
