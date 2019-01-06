assert_success jobserver init true
assert_success jobserver init jobserver started
assert_fails   jobserver init jobserver started true
