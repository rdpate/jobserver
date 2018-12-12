for x in started release acquire sleep-start sleep-end; do
    assert_fails "$jobserver" "$x" 2>/dev/null
    done
assert_fails "$jobserver" init
assert_fails "$jobserver" init "$jobserver" init
assert_success "$jobserver" init "$jobserver" started
assert_success "$jobserver" init "$jobserver" init "$jobserver" started
