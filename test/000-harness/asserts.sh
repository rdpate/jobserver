assert_success true
if (assert_success false) >/dev/null; then
    failed 'assert_success did not detect failure'
    fi

assert_fails false
if (assert_fails true) >/dev/null; then
    failed 'assert_fails did not detect success'
    fi

empty=
value='a b'

assert_empty empty
if (assert_empty value) >/dev/null; then
    failed 'assert_empty passed on non-empty value'
    fi

assert_nonempty value
if (assert_nonempty empty) >/dev/null; then
    failed 'assert_nonempty passed on empty value'
    fi

assert_equals '' ''
assert_equals 'a b' 'a b'
if (assert_equals 'a b' 'a bc') >/dev/null; then
    failed 'assert_equals passed on different values'
    fi

assert_different a b
if (assert_different a a) >/dev/null; then
    failed 'assert_different passed on different values'
    fi
