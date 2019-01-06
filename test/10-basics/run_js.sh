assert_equals  10 "$(seq 1 10 | "$project/doc/xlines" echo | wc -l)"

assert_fails   JOBSERVER_FDS=3,3 jobsv-sh started 3<&-

assert_equals  x "$(jobsv-sh release echo x & jobsv-sh acquire; wait)"
assert_equals  x "$(echo a | jobsv-sh release sh -c 'read v; echo ${v:-x}' & jobsv-sh acquire; wait)"
