#! /bin/bash
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
  echo "Usage: `basename $0` <test_run_directory>  <destination_directory>"
  echo ""
  echo "test_run_directory      This is the path containing test run files to process."
  echo ""
  echo "destination_directory   This is the location of the directory"
  echo "                        for the generated images."
  echo ""
  echo "Options must be specified in the order shown above."
  echo ""
  echo "Examples:"
  echo "`basename $0` 2021-06-28T21:07:53+0000_2b84581bca05b99ad8287e608193e1f39d4a1a59_d41d8cd98f00b204e9800998ecf8427e /var/www/html/perf/images"
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
  if [ $# -gt 1 ]; then
    TEST_RUN_DIR=$1
    OUTPUT_DIR=$2

    if ! [ -d "$TEST_RUN_DIR" ]; then
      echo "Test Run Directory $TEST_RUN_DIR Does Not Exist."
      usage
      exit
    fi
  else
    usage
    exit
  fi
  echo "TEST_RUN_DIR : $TEST_RUN_DIR"
  echo "OUTPUT_DIR : $OUTPUT_DIR"
}


###############################################################################
#
# plot_latency_results
#
# Generate the graphs for the latency tests.
#
###############################################################################

declare -a protocols=("tcp" "udp" "multi-be" "multi-re" "rtps" ) #"raw-tcp" "raw-udp")
declare -a protocol_descs=('TCP' 'UDP' 'Best Effort Multicast' 'Reliable Multicast' 'RTPS') #'Raw TCP' 'Raw UDP')

plot_latency_results ()
{
  local DATA_DIR="$TEST_RUN_DIR/data"

  if [ -e "$DATA_DIR/latency.csv" ]; then
    # Plotting summary charts
    gnuplot <<EOF
      call "$SCRIPT_DIR/plot-transports.gpi"  "$DATA_DIR/latency.csv" "$OUTPUT_DIR/transport-latency"
      call "$SCRIPT_DIR/plot-jitter.gpi"  "$DATA_DIR/latency.csv" "$OUTPUT_DIR/transport-jitter"
      exit
EOF

    for (( i=0; i<${#protocols[@]}; i++ ));
    do
      protocol=${protocols[$i]}
      desc=${protocol_descs[$i]}
      for sz in 50 100 250 500 1000 2500 5000 8000 16000 32000
      do
        if [ -f "$DATA_DIR/latency-$protocol-$sz.gpd" ]; then
          gnuplot <<EOF
            call "$SCRIPT_DIR/lj-plots.gpi" "$DATA_DIR/latency-$protocol-$sz.gpd" "$DATA_DIR/latency-$protocol-$sz.stats" "$OUTPUT_DIR/latency-$protocol-$sz.png" "$desc / Message Size $sz bytes"
           exit
EOF
        fi
      done
    done

    # Plotting Kernel Density Estimates
    # gnuplot 4.4 functionality in the plot-density.gpi fails on earlier systems.
    if [ "$GNUPLOT_MAJORVERSION" -gt 4 -a "$GNUPLOT_MAJORVERSION" -gt 0 ]; then
    # Plotting Quantile Distributions
      gnuplot <<EOF
        call "$SCRIPT_DIR/plot-quantiles.gpi" "$DATA_DIR"  "$OUTPUT_DIR"
        exit
EOF


      gnuplot <<EOF
        call "$SCRIPT_DIR/plot-density.gpi" "$DATA_DIR"  "$OUTPUT_DIR"
        exit
EOF
    fi

  else
    echo "Missing latency data file $DATA_DIR/latency.csv"
    echo " No latency plots run"
  fi
}


###############################################################################
#
# plot_throughput_results
#
# Generate the graphs for the thru tests.
#
###############################################################################
plot_throughput_results ()
{
  local DATA_DIR="$TEST_RUN_DIR/data"

  if [ -e "$DATA_DIR/throughput.csv" ]; then
    gnuplot <<EOF
      # Plotting test format charts
      call "$SCRIPT_DIR/plot-throughput-testformats.gpi"  "$DATA_DIR/throughput.csv" "$OUTPUT_DIR"
      # Plotting transport charts
      call "$SCRIPT_DIR/plot-throughput-transports.gpi"  "$DATA_DIR/throughput.csv" "$OUTPUT_DIR"
      call "$SCRIPT_DIR/plot-throughput-transports.gpi"  "$DATA_DIR/throughput.csv" "$OUTPUT_DIR" "smooth acspline"
      exit
EOF
  else
    echo "Missing throughput data file $DATA_DIR/throughput.csv"
    echo " No throughput plots run"
  fi
}


OUTPUT_DIR="."
TEST_RUN_DIR="."
GNUPLOT_MAJORVERSION=`gnuplot --version | sed -n -e "s/gnuplot \([0-9]*\).[0-9]* patchlevel.*/\1/gp"`
GNUPLOT_MINORVERSION=`gnuplot --version | sed -n -e "s/gnuplot [0-9]*.\([0-9]*\) patchlevel.*/\1/gp"`

parse_input $@

plot_latency_results

# plot_throughput_results
