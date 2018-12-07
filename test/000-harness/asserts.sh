assert_success 1 true
if (assert_success 1 false) >/dev/null; then
    fail 2 'assert_success did not detect failure'
    fi

assert_fails 3 false
if (assert_fails 1 true) >/dev/null; then
    fail 4 'assert_fails did not detect success'
    fi

empty=
value='a b'

assert_empty 5 empty
if (assert_empty 1 value) >/dev/null; then
    fail 6 'assert_empty passed on non-empty value'
    fi

assert_nonempty 7 value
if (assert_nonempty 1 empty) >/dev/null; then
    fail 8 'assert_nonempty passed on empty value'
    fi

assert_equals 9 '' ''
assert_equals 10 'a b' 'a b'
if (assert_equals 1 'a b' 'a bc') >/dev/null; then
    fail 11 'assert_equals passed on different values'
    fi

