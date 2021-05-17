set -e

# export PATH="/data/work/oci/cmake-3.3.2-Linux-x86_64/bin:$PATH"
source /data/work/oci/dds/PyOpenDDS/setenv_no_opendds.sh

rm -fr build ../../DevGuideExamples/DCPS/Messenger/build/

mkdir build
cd build
cmake ..
cmake --build . -- -j 8

# echo 'set(CTEST_CUSTOM_MAXIMUM_PASSED_TEST_OUTPUT_SIZE 4294967296)' > CTestCustom.cmake
# echo 'set(CTEST_CUSTOM_MAXIMUM_FAILED_TEST_OUTPUT_SIZE 4294967296)' >> CTestCustom.cmake
# ctest --no-compress-output -T Test
# ctest
cd ../..
./auto_run_tests.pl --no-dcps --cmake
