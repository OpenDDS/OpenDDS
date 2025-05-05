# By default test output is cut off at 1KiB. There doesn't seem to be a way to
# disable the cut off, so change it to 1GiB.
set(CTEST_CUSTOM_MAXIMUM_PASSED_TEST_OUTPUT_SIZE 1073741824)
set(CTEST_CUSTOM_MAXIMUM_FAILED_TEST_OUTPUT_SIZE 1073741824)
