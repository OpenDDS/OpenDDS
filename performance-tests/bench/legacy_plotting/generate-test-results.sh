#! /bin/sh
#
#
BENCH_ROOT="$DDS_ROOT/performance-tests/bench"
SCRIPT_DIR="$BENCH_ROOT/legacy_plotting"


###############################################################################
#
# usage
#
###############################################################################
usage ()
{
  echo "Usage: `basename $0` <test_run_directory>"
  echo ""
  echo "test_run_directory      This is the path containing test run files to process."
  echo ""
  echo "Examples:"
  echo "`basename $0` 2021-06-28T21:07:53+0000_2b84581bca05b99ad8287e608193e1f39d4a1a59_d41d8cd98f00b204e9800998ecf8427e"
  exit
}


###############################################################################
#
# parse_input
#
# Parse the commandline and validate the inputs
#
###############################################################################
parse_input ()
{
  if [ $# -gt 0 ]; then
    TEST_RUN_DIR=$1

    if ! [ -d "$TEST_RUN_DIR" ]; then
      echo "Test Run Directory $TEST_RUN_DIR Does Not Exist."
      usage
    fi
  else
    usage
  fi
  echo "TEST_RUN_DIR : $TEST_RUN_DIR"
}


###############################################################################
#
# process_latency_test
#
# Generate the stats for the latency tests.
#
###############################################################################
process_latency_test ()
{
  local DATA_DIR="$TEST_RUN_DIR/data"

  mkdir -p "$DATA_DIR"

  for sz in 50 100 250 500 1000 2500 5000 8000 16000 32000
  do
    for protocol in tcp udp multicast-be multicast-rel rtps # raw-udp raw-tcp
    do
      $BENCH_ROOT/report_parser/report_parser --input-file "$TEST_RUN_DIR/b1_latency_${protocol}_${sz}.json" --output-file "$DATA_DIR/latency-$protocol-$sz.data" --output-type time-series --output-format gnuplot --stats round_trip_latency
      $SCRIPT_DIR/reduce-latency-data.pl "$DATA_DIR/latency-$protocol-$sz.data" > "$DATA_DIR/latency-$protocol-$sz.gpd"
    done
  done

  $SCRIPT_DIR/extract-latency.pl "$DATA_DIR"/latency-*.gpd > "$DATA_DIR/latency.csv"
  $SCRIPT_DIR/extract-latency-json.pl "$DATA_DIR"/latency-*.gpd > "$DATA_DIR/latency.csv.js"
  $SCRIPT_DIR/gen-latency-stats.pl "$DATA_DIR/latency.csv"
}


###############################################################################
#
# process_throughput_test
#
# Generate the stats for the thru tests.
#
###############################################################################
process_throughput_test ()
{
  local DATA_DIR="$TEST_RUN_DIR/data"

  mkdir -p "$DATA_DIR"

  $SCRIPT_DIR/extract-throughput.pl "$TEST_RUN_DIR"/*/*.results > "$DATA_DIR/throughput.csv"
}

TEST_RUN_DIR="."

parse_input $@

process_latency_test

# process_throughput_test
