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

args=($@)

cd "${DDS_ROOT}"

function get-units {
    if [[ ${#args[@]} == 0 ]]
    then
        find FACE dds tools -path tools/rapidjson -prune -false -o -path tools/modeling/tests -prune -false -o -name "*.h" -o -name "*.cpp" -o -name "*.inl" |
            sed -E -e '/[CS]\.(h|cpp|inl)$/ d' -e '/TypeSupportImpl\.(h|cpp)$/ d' -e '/[Ee]xport.h$/ d' -e 's/\.[^.]*$//' |
            sort -u
    else
        echo "${args[@]}"
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
    lcov --capture --directory . --output-file coverage.info --no-external &&
    lcov --extract coverage.info --output-file coverage.info "*${unit_name}.*" &&
    if [ -e coverage-total.info ]
    then
        lcov --add-tracefile coverage.info --add-tracefile coverage-total.info --output-file coverage-total.info
    else
        mv coverage.info coverage-total.info
    fi
done

lcov --remove coverage-total.info --output-file coverage-total.info '*/tests/unit-tests/*'

# Generate HTML
rm -rf coverage-out
genhtml coverage-total.info --output-directory coverage-out
