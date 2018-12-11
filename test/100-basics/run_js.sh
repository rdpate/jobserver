x="$(seq 1 10 | "$project/doc/xlines" echo | wc -l)"
assert_equals 1 "$x" 10
