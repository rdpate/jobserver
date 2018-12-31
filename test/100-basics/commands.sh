# fail if not run under Jobserver
assert_fails    jobserver started
assert_fails    jobserver release
assert_fails    jobserver acquire
assert_fails    jobserver sleep-start
assert_fails    jobserver sleep-end

# CMD not optional
assert_fails    jobserver init
# still not optional when reusing pool
assert_fails    jobserver init jobserver init

# started succeeds
assert_success  jobserver init jobserver started
# even when nested
assert_success  jobserver init jobserver init jobserver started

# sync-exec without active pool is not an error
assert_success  jobserver sync-exec true
# not started within sync-exec
assert_fails    jobserver init jobserver sync-exec jobserver started
