set -Cue
exec >&2
redo-ifchange "../src/$1"
ln "../src/$1" "$3"
