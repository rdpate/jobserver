for x in started release acquire sleep-start sleep-end; do
    assert_fails "$jobserver" "$x" 2>/dev/null
    done
# CMD not optional
assert_fails "$jobserver" init
# still not optional when reusing pool
assert_fails "$jobserver" init "$jobserver" init
# started succeeds
assert_success "$jobserver" init "$jobserver" started
# even when nested
assert_success "$jobserver" init "$jobserver" init "$jobserver" started

# sync-exec without active pool is not an error
assert_success "$jobserver" sync-exec true
# not started within sync-exec
assert_fails "$jobserver" init "$jobserver" sync-exec "$jobserver" started
