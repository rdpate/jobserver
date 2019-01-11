exec >&2
set -Cue
output="$3"
set -- jobserver.o jobserver_fatal.o jobserver_nonblock_read.o
redo-ifchange "$@"
ar r "$output" "$@"
