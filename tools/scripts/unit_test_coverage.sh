#!/bin/bash

# This script generates a unit test coverage report for OpenDDS.  It
# does not generate an intentional unit test coverage report due to
# deficiencies in the tools involved.  It requires lcov and a
# compatible compiler like gcc or clang.  A unit is a source code
# module consisting of a *.h, *.cpp, and/or *.inl file.  The unit is
# named by the path to the prefix, e.g., FACE/Sequence.  With no
# arguments, it generates a coverage report for all units.  Otherwise,
# the arguments are interpreted as a list of units.  Thus, if you
# want to improve the coverage for a single unit, the script can be
# used to geneerate the coverage for that unit in isolation.

start=$(date +'%s')

args=($@)

cd "${DDS_ROOT}"

function get-units {
    for arg in "${args[@]}"
    do
        echo "${arg%.*}"
    done | sort -u
}

function get-gcda {
    local -r unit_name="$1"
    for path in $(find FACE dds tools tests/unit-tests -path tools/rapidjson -prune -false -o -name "*.gcda")
    do
        local derived_unit_name=$(echo "${path}" | sed -e 's@/\.shobj@@g' -e 's@/\.obj@@g' -e 's@^tests/unit-tests/@@' -e 's@\.gcda$@@' -e 's@/@_@g')
        if [[ ${unit_name} == ${derived_unit_name} ]]
        then
            echo "${path}"
        fi
    done
}

result=0

echo '=============================================================================='
echo 'auto_run_tests: tools/scripts/unit_test_coverage.sh'
echo ''

lcov --zerocounters --directory .

if [[ ${#args[@]} == 0 ]]
then
    echo "Testing coverage of all unit tests"
    (
        cd "${DDS_ROOT}/tests/unit-tests"
        ./UnitTests
    ) || result=1
else
    for unit_name in $(get-units)
    do
        echo "Testing coverage of ${unit_name}"

        # Compute the test label.
        prefix=$(echo "${unit_name}" | sed -e 's@/@_@g')

        # Run the unit test with label.
        (
            cd "${DDS_ROOT}/tests/unit-tests"
            ./UnitTests --gtest_filter="${prefix}*"
        ) || result=1
    done
fi

# Collect data.
rm -f coverage.info
lcov --capture --directory . --output-file coverage.info --no-external
lcov --remove coverage.info --output-file coverage.info '*tests/*' '*/ACE/*' '*/TAO/*' '*/rapidjson/*' '*/yard/*' '*C.h' '*C.inl' '*C.cpp' '*TypeSupportImpl.h' '*TypeSupportImpl.cpp'

# Generate HTML
rm -rf coverage-out
genhtml coverage.info --output-directory coverage-out

stop=$(date +'%s')

if [[ ${result} == 0 ]]
then
    echo 'test PASSED.'
else
    echo 'test FAILED.'
fi

echo ''
echo "auto_run_tests: tools/scripts/unit_test_coverage.sh Time:$(($stop - $start))s Result:${result}"

exit $result
