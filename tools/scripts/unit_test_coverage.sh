#!/bin/bash

# This script generates an intentional unit test coverage report for
# OpenDDS.  It requires lcov and a compatible compiler like gcc or
# clang.  A unit is a source code module consisting of a *.h, *.cpp,
# and/or *.inl file.  The unit is named by the path to the prefix,
# e.g., FACE/Sequence.  With no arguments, it generates a coverage
# report for all units.  Otherwise, the arguments are interpretted as
# a list of units.  Thus, if you want to improve the coverage for a
# single unit, the script can be used to geneerate the coverage for
# that unit in isolation.

set -e

start=$(date +'%s')

args=($@)

cd "${DDS_ROOT}"

function get-all-files {
    git ls-files |
        sed -n -E \
            -e '/^(FACE|dds|tools)/! d' \
            -e '/^tools\/modeling\/tests/ d' \
            -e '/\.(h|cpp|inl)$/! d' \
            -e '/[Ex]port\.h$/ d' \
            -e 'p'
}

function get-units {
    if [[ ${#args[@]} == 0 ]]
    then
        get-all-files | sed -E -e 's/\.[^.]*$//' | sort -u
    else
        for arg in "${args[@]}"
        do
            if [[ -d "${arg}" ]]
            then
                get-all-files | grep "${args}" | sed -E -e 's/\.[^.]*$//' | sort -u
            else
                echo "${arg}"
            fi
        done
    fi
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

rm -f coverage-total.info

for unit_name in $(get-units)
do
    echo "Testing coverage of ${unit_name}"

    # Compute the test label.
    prefix=$(echo "${unit_name}" | sed -e 's@/@_@g')

    # Clean up.
    lcov --zerocounters --directory .

    # Run the unit test with label.
    (
        cd "${DDS_ROOT}/tests/unit-tests"
        output=$(./UnitTests --gtest_filter="${prefix}*" 2>&1)
        echo "${output}"
        if [[ ${output} == *"0 tests from 0 test cases ran"* ]]
        then
            echo "ERROR: No test cases for ${unit_name}"
            result=1
        fi
    )

    # Delete gcda files not related to the unit.
    for path in $(find . -name "*.gcda")
    do
        derived_unit_name=$(echo "${path}" | sed -e 's@^\./@@' -e 's@/\.shobj@@g' -e 's@/\.obj@@g' -e 's@^tests/unit-tests/@@' -e 's@\.gcda$@@')
        if [[ ${derived_unit_name} != ${unit_name} ]]
        then
            rm $path
        else
            echo "Considering ${path} for ${unit_name}"
        fi
    done

    # Collect data.
    rm -f coverage.info
    { lcov --capture --directory . --output-file coverage.info --no-external &&
          lcov --extract coverage.info --output-file coverage.info "*${unit_name}.*" &&
          [ -s coverage.info ] &&
          if [ -e coverage-total.info ]
          then
              lcov --add-tracefile coverage.info --add-tracefile coverage-total.info --output-file coverage-total.info
          else
              mv coverage.info coverage-total.info
          fi ;
    } || true
done

lcov --remove coverage-total.info --output-file coverage-total.info '*/tests/unit-tests/*'

# Generate HTML
rm -rf coverage-out
genhtml coverage-total.info --output-directory coverage-out

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
