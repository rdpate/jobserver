# fail if not run under Jobserver
assert_fails    jobsv-sh started
assert_fails    jobsv-sh release
assert_fails    jobsv-sh acquire

# CMD not optional
assert_fails    jobsv
# still not optional when reusing pool
assert_fails    jobsv jobsv

# started succeeds
assert_success  jobsv jobsv-sh started

# child return propagated
assert_fails    jobsv false
assert_exitcode 42 jobsv sh -c 'exit 42'
assert_exitcode 143 jobsv sh -c 'kill $$'

# init options
assert_success  jobsv --new true
assert_success  jobsv --new=4 true
assert_success  jobsv --fixed true
assert_success  jobsv --fixed=4 true

assert_fails    jobsv --new= true
assert_fails    jobsv --fixed= true

# arguments required
assert_fails    jobsv jobsv-sh release
# arguments forbidden
assert_fails    jobsv jobsv-sh started true
assert_fails    jobsv jobsv-sh acquire true
