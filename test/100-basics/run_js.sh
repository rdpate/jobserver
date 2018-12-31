assert_equals  10 "$(seq 1 10 | "$project/doc/xlines" echo | wc -l)"

assert_success jobserver started --debug
assert_fails   jobserver started --debug true

assert_fails   JOBSERVER_FDS=3,3 jobserver started 3<&-

assert_equals  x "$(jobserver release echo x & jobserver acquire; wait)"
assert_equals  x "$(echo a | jobserver release sh -c 'read v; echo ${v:-x}' & jobserver acquire; wait)"
