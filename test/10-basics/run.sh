assert_success jobsv true
assert_success jobsv jobsv-sh started
assert_fails   jobsv jobsv-sh started true
