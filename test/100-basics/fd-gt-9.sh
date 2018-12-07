# 0,1,2 are stdin,out,err; requiring 8 more FDs means we reach at least FD 10
assert_success 1 \
    "$jobserver" init --new \
    "$jobserver" init --new \
    "$jobserver" init --new \
    "$jobserver" init --new --debug true
