#!/bin/bash
export BENCH_ROOT=${DDS_ROOT}/performance-tests/bench
export BENCH_BUILDER_ROOT=${DDS_ROOT}/performance-tests/bench
export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${BENCH_BUILDER_ROOT}/lib:${BENCH_ROOT}/lib
