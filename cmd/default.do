exec >&2
redo-ifchange "$1".c
cc -O2 -o"$3" "$1".c
#strip "$3"
