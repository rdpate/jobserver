# 0,1,2 are stdin,out,err; requiring 8 more FDs means we reach at least FD 10
# this tests that FDs >9 work
assert_success \
    jobserver init --new \
    jobserver init --new \
    jobserver init --new \
    jobserver init --new \
    sh -c 'jobserver release true; jobserver acquire'
