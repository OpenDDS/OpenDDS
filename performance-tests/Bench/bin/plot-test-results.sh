#! /bin/bash
#
#

SCRIPT_DIR=$( cd "$( dirname ${BASH_SOURCE[0]} )" && pwd )


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

declare -a protocols=("tcp" "udp" "multi-be" "multi-re" "rtps" "raw-tcp" "raw-udp")
declare -a protocol_descs=('TCP' 'UDP' 'Best Effort Multicast' 'Reliable Multicast' 'RTPS' 'Raw TCP' 'Raw UDP')

plot_latency_results ()
{
  local DATADIR="$BASEDIR/tests/latency/data"

  if [ -e "$DATADIR/latency.csv" ]; then
    # Plotting summary charts
    gnuplot <<EOF
      call "$SCRIPT_DIR/plot-transports.gpi"  "$DATADIR/latency.csv" "$OUTDIR/transport-latency"
      call "$SCRIPT_DIR/plot-jitter.gpi"  "$DATADIR/latency.csv" "$OUTDIR/transport-jitter"
      exit
EOF

    for (( i=0; i<${#protocols[@]}; i++ ));
    do
      protocol=${protocols[$i]}
      desc=${protocol_descs[$i]}
      for sz in 50 100 250 500 1000 2500 5000 8000 16000 32000
      do
        if [ -f "$DATADIR/latency-$protocol-$sz.gpd" ]; then
          gnuplot <<EOF
            call "$SCRIPT_DIR/lj-plots.gpi" "$DATADIR/latency-$protocol-$sz.gpd" "$DATADIR/latency-$protocol-$sz.stats" "$OUTDIR/latency-$protocol-$sz.png" "$desc / Message Size $sz bytes"
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
        call "$SCRIPT_DIR/plot-quantiles.gpi" "$DATADIR"  "$OUTDIR"
        exit
EOF


      gnuplot <<EOF
        call "$SCRIPT_DIR/plot-density.gpi" "$DATADIR"  "$OUTDIR"
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
      call "$SCRIPT_DIR/plot-throughput-testformats.gpi"  "$DATADIR/throughput.csv" "$OUTDIR"
      # Plotting transport charts
      call "$SCRIPT_DIR/plot-throughput-transports.gpi"  "$DATADIR/throughput.csv" "$OUTDIR"
      call "$SCRIPT_DIR/plot-throughput-transports.gpi"  "$DATADIR/throughput.csv" "$OUTDIR" "smooth acspline"
      exit
EOF
  else
    echo "Missing throughput data file $DATADIR/throughput.csv"
    echo " No throughput plots run"
  fi
}



OUTDIR="."
BASEDIR="."
GNUPLOT_MAJORVERSION=`gnuplot --version | sed -n -e "s/gnuplot \([0-9]*\).[0-9]* patchlevel.*/\1/gp"`
GNUPLOT_MINORVERSION=`gnuplot --version | sed -n -e "s/gnuplot [0-9]*.\([0-9]*\) patchlevel.*/\1/gp"`

parse_input $@

plot_latency_results

plot_throughput_results

