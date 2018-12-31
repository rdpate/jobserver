assert_success jobserver init true
assert_success jobserver init jobserver started
assert_fails   jobserver init jobserver started true

assert_fails   jobserver-makeflags
assert_fails   jobserver-makeflags --dup

assert_success jobserver init jobserver-makeflags
assert_success jobserver init jobserver-makeflags true
assert_fails   jobserver init jobserver-makeflags --dup
assert_success jobserver init jobserver-makeflags --dup true
