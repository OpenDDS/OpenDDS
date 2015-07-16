#! /bin/sh
#
#



###############################################################################
#
# usage
#
###############################################################################
usage ()
{
  echo "Usage: `basename $0` <bench_directory>"
  echo ""
  echo "bench_directory         This is the location of the Bench performance"
  echo "                        tests directory."
  echo ""
  echo "Examples:"
  echo "`basename $0` $DDS_ROOT/performance-tests/Bench"
  echo "`basename $0` /home/tester/perf-tests"
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
    BASEDIR=$1

    if ! [ -d "$BASEDIR" ]; then
      echo "bench_directory $BASEDIR does not exist."
      usage
    fi
  else
    usage
  fi
  echo "BASEDIR : $BASEDIR"
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
  local TESTDIR="$BASEDIR/tests/latency"
  local DATADIR="$TESTDIR/data"

  mkdir -p "$DATADIR"

  for sz in 50 100 250 500 1000 2500 5000 8000 16000 32000
  do
    $DDS_ROOT/performance-tests/Bench/bin/reduce-latency-data.pl "$TESTDIR/tcp/latency-$sz.data" > "$DATADIR/latency-tcp-$sz.gpd"
    $DDS_ROOT/performance-tests/Bench/bin/reduce-latency-data.pl "$TESTDIR/udp/latency-$sz.data" > "$DATADIR/latency-udp-$sz.gpd"
    $DDS_ROOT/performance-tests/Bench/bin/reduce-latency-data.pl "$TESTDIR/multi-be/latency-$sz.data" > "$DATADIR/latency-mbe-$sz.gpd"
    $DDS_ROOT/performance-tests/Bench/bin/reduce-latency-data.pl "$TESTDIR/multi-rel/latency-$sz.data" > "$DATADIR/latency-mrel-$sz.gpd"
    $DDS_ROOT/performance-tests/Bench/bin/reduce-latency-data.pl "$TESTDIR/rtps/latency-$sz.data" > "$DATADIR/latency-rtps-$sz.gpd"
  done

  $DDS_ROOT/performance-tests/Bench/bin/extract-latency.pl "$DATADIR"/latency-*.gpd > "$DATADIR/latency.csv"
  $DDS_ROOT/performance-tests/Bench/bin/gen-latency-stats.pl "$DATADIR/latency.csv"
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
  local TESTDIR="$BASEDIR/tests/thru"
  local DATADIR="$TESTDIR/data"

  mkdir -p "$DATADIR"

  $DDS_ROOT/performance-tests/Bench/bin/extract-throughput.pl "$TESTDIR"/*/*.results > "$DATADIR/throughput.csv"
}

BASEDIR="."

parse_input $@

process_latency_test

process_throughput_test
