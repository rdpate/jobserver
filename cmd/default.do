exec >&2
case "$1" in
    all) exec redo-ifchange _init2 ;;
    _init2) ;;
    *) exit 1 ;;
    esac
redo-ifchange "$1".c
cc -O2 -o"$3" "$1".c
#strip "$3"
