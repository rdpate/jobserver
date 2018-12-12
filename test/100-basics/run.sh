assert_success jobserver init true
assert_success jobserver init --debug true
assert_success jobserver init jobserver started --debug
assert_fails   jobserver init jobserver started --debug true
