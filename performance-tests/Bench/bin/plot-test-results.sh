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
  echo "Usage: `basename $0` <bench_directory> <destination_directory>"
  echo ""
  echo "bench_directory         This is the location of the Bench performance"
  echo "                        tests directory."
  echo ""
  echo "destination_directory   This is the location of the directory"
  echo "                         for the generated images."
  echo ""
  echo "Options must be specified in the order shown above."
  echo ""
  echo "Examples:"
  echo "`basename $0` $DDS_ROOT/performance-tests/Bench /var/www/html/perf/images"
  echo "`basename $0` /home/tester/perf-tests /home/tester/perf-results/images"
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
    BASEDIR=$1
    OUTDIR=$2

    if ! [ -d "$BASEDIR" ]; then
      echo "bench_directory $BASEDIR does not exist."
      usage
    fi
  else
    usage
    exit
  fi
  echo "BASEDIR : $BASEDIR"
  echo "OUTDIR : $OUTDIR"
}


###############################################################################
#
# plot_latency_results
#
# Generate the graphs for the latency tests.
#
###############################################################################
plot_latency_results ()
{
  local DATADIR="$BASEDIR/tests/latency/data"

  if [ -e "$DATADIR/latency.csv" ]; then
    # Plotting summary charts
    gnuplot <<EOF
      call "$DDS_ROOT/performance-tests/Bench/bin/plot-transports.gpi"  "$DATADIR/latency.csv" "$OUTDIR/transport-latency"
      call "$DDS_ROOT/performance-tests/Bench/bin/plot-jitter.gpi"  "$DATADIR/latency.csv" "$OUTDIR/transport-jitter"
      exit
EOF

    for sz in 50 100 250 500 1000 2500 5000 8000 16000 32000
    do
      gnuplot <<EOF
        # Plotting TCP charts
        call "$DDS_ROOT/performance-tests/Bench/bin/lj-plots.gpi" "$DATADIR/latency-tcp-$sz.gpd" "$DATADIR/latency-tcp-$sz.stats" "$OUTDIR/latency-tcp-$sz.png" "TCP / Message Size $sz bytes"
        # Plotting UDP charts
        call "$DDS_ROOT/performance-tests/Bench/bin/lj-plots.gpi" "$DATADIR/latency-udp-$sz.gpd" "$DATADIR/latency-udp-$sz.stats" "$OUTDIR/latency-udp-$sz.png" "UDP / Message Size $sz bytes"
        # Plotting Multicast / best effort charts
        call "$DDS_ROOT/performance-tests/Bench/bin/lj-plots.gpi" "$DATADIR/latency-mbe-$sz.gpd" "$DATADIR/latency-mbe-$sz.stats" "$OUTDIR/latency-mbe-$sz.png" "Multicast - Best Effort / Message Size $sz bytes"
        # Plotting Multicast / reliable charts
        call "$DDS_ROOT/performance-tests/Bench/bin/lj-plots.gpi" "$DATADIR/latency-mrel-$sz.gpd" "$DATADIR/latency-mrel-$sz.stats" "$OUTDIR/latency-mrel-$sz.png" "Multicast - Reliable / Message Size $sz bytes"
        # Plotting RTPS charts
        call "$DDS_ROOT/performance-tests/Bench/bin/lj-plots.gpi" "$DATADIR/latency-rtps-$sz.gpd" "$DATADIR/latency-rtps-$sz.stats" "$OUTDIR/latency-rtps-$sz.png" "RTPS / Message Size $sz bytes"
       exit
EOF
    done


    # Plotting Quantile Distributions
    gnuplot <<EOF
      call "$DDS_ROOT/performance-tests/Bench/bin/plot-quantiles.gpi" "$DATADIR"  "$OUTDIR"
      exit
EOF

    # Plotting Kernel Density Estimates
    # gnuplot 4.4 functionality in the plot-density.gpi fails on earlier systems.
    if [ "$GNUPLOT_MAJORVERSION" -gt 3 -a "$GNUPLOT_MAJORVERSION" -gt 2 ]; then
      gnuplot <<EOF
        call "$DDS_ROOT/performance-tests/Bench/bin/plot-density.gpi" "$DATADIR"  "$OUTDIR"
        exit
EOF
    fi

  else
    echo "Missing latency data file $DATADIR/latency.csv"
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
  local DATADIR="$BASEDIR/tests/thru/data"

  if [ -e "$DATADIR/throughput.csv" ]; then
    gnuplot <<EOF
      # Plotting test format charts
      call "$DDS_ROOT/performance-tests/Bench/bin/plot-throughput-testformats.gpi"  "$DATADIR/throughput.csv" "$OUTDIR"
      # Plotting transport charts
      call "$DDS_ROOT/performance-tests/Bench/bin/plot-throughput-transports.gpi"  "$DATADIR/throughput.csv" "$OUTDIR"
      call "$DDS_ROOT/performance-tests/Bench/bin/plot-throughput-transports.gpi"  "$DATADIR/throughput.csv" "$OUTDIR" "smooth acspline"
      exit
EOF
  else
    echo "Missing throughput data file $DATADIR/throughput.csv"
    echo " No throughput plots run"
  fi
}



OUTDIR="."
BASEDIR="."
GNUPLOT_MAJORVERSION=`gnuplot --version | sed -n -e "s/gnuplot \([0-9]*\).[0-9]* patchlevel.*/\1/gpi"`
GNUPLOT_MINORVERSION=`gnuplot --version | sed -n -e "s/gnuplot [0-9]*.\([0-9]*\) patchlevel.*/\1/gpi"`

parse_input $@

plot_latency_results

plot_throughput_results

