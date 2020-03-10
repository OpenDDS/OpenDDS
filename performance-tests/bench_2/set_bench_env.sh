#!/bin/bash
export BENCH_ROOT=${DDS_ROOT}/performance-tests/bench_2
export BENCH_BUILDER_ROOT=${DDS_ROOT}/performance-tests/bench_2
export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${BENCH_BUILDER_ROOT}/lib:${BENCH_ROOT}/lib
