set -Cue
exec >&2
fatal() { rc="$1"; shift; printf %s\\n "${0##*/} error: $*" >&2 || true; exit "$rc"; }

src="${1%.o}.c"
redo-ifchange "$src"
[ -e "$src" ] || fatal 66 "missing source $1"
dep="$(dirname "$1")/.$(basename "$1").dep"
cc -c -Wall -Werror -MD -MF "$dep" -O3 -o"$3" "$src"
sed -i -r 's/^[^ ]+: //; s/ \\$//; s/^ +| +$//g; s/ +/\n/g' "$dep"
xargs redo-ifchange <"$dep"
rm "$dep"
