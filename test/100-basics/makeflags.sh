# check that --dup gives different FDs
x="$(jobserver init jobserver-makeflags       printenv MAKEFLAGS)"
y="$(jobserver init jobserver-makeflags --dup printenv MAKEFLAGS)"
assert_different "$x" "$y"
