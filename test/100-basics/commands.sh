for x in started release acquire sleep-start sleep-end; do
    assert_fails 1 "$jobserver" "$x" 2>/dev/null
    done
assert_fails 1 "$jobserver" init
assert_fails 1 "$jobserver" init "$jobserver" init
assert_success 1 "$jobserver" init "$jobserver" started
assert_success 1 "$jobserver" init "$jobserver" init "$jobserver" started

assert_success 1 "$jobserver" sync-exec true
assert_fails 1 "$jobserver" init "$jobserver" sync-exec "$jobserver" started
