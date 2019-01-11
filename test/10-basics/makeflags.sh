# MAKEFLAGS exported by default
assert_success  env -uMAKEFLAGS jobsv printenv MAKEFLAGS
# even if already exists
assert_different 'xyz' "$(MAKEFLAGS=xyz jobsv printenv MAKEFLAGS)"
assert_equals   'xyz' "$(MAKEFLAGS=xyz jobsv printenv MAKEFLAGS | cut -b1-3)"
# but can be disabled
assert_fails    env -uMAKEFLAGS jobsv --no-makeflags printenv MAKEFLAGS
