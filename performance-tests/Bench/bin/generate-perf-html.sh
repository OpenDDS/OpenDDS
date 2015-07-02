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
  echo "destination_directory   This designates the location of the generated"
  echo "                        html."
  echo ""
  echo "Options must be specified in the order shown above."
  echo ""
  echo "Examples:"
  echo "`basename $0` $DDS_ROOT/performance-tests/Bench /var/www/html/perf"
  echo "`basename $0` /home/tester/perf-tests /home/tester/perf-results"
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
  fi
  echo "BASEDIR : $BASEDIR"
  echo "OUTDIR : $OUTDIR"
}


###############################################################################
#
# create_output_directory
#
# Create the output directory and subdirectories
#
###############################################################################
create_output_directory ()
{
  if ! [ -d "$OUTDIR" ]; then
    mkdir -p "$OUTDIR"
    echo "Created  $OUTDIR"
  fi

  if ! [ -d "$OUTDIR/images" ]; then
    mkdir -p "$OUTDIR/images"
    echo "Created  $OUTDIR/images"
  fi

  if ! [ -d "$OUTDIR/images/thumbnails" ]; then
    mkdir -p "$OUTDIR/images/thumbnails"
    echo "Created  $OUTDIR/images/thumbnails"
  fi

  if ! [ -e "$OUTDIR/formatting.css" ]; then
    cp "$BASEDIR/tools/formatting.css" "$OUTDIR/formatting.css"
  fi
}


###############################################################################
#
# store_result_files
#
# Tar and gzip the test result files and place in the output directory.
#
###############################################################################
store_result_files ()
{
  pushd "$BASEDIR"

  tar --ignore-failed-read -czf "$OUTDIR/perf_tests_logs_$DATE.tar.gz" \
    tests/latency/tcp \
    tests/latency/udp \
    tests/latency/multi-rel \
    tests/latency/multi-be \
    tests/latency/rtps \
    tests/thru/tcp \
    tests/thru/udp \
    tests/thru/multi-rel \
    tests/thru/multi-be \
    tests/thru/rtps

  popd
}


###############################################################################
#
# process_latency_test
#
# Generate the tables for the latency test.
#
###############################################################################
process_latency_test ()
{
  $DDS_ROOT/performance-tests/Bench/bin/gen-latency-tables.pl "$BASEDIR/tests/latency/data/latency.csv"
}


###############################################################################
#
# process_throughput_test
#
# Generate the tables for the thru test.
#
###############################################################################
process_throughput_test ()
{
  $DDS_ROOT/performance-tests/Bench/bin/gen-throughput-tables.pl "$BASEDIR/tests/thru/data/throughput.csv"
}


###############################################################################
#
# process_images
#
# Generate thumbnails for the images
#
###############################################################################
process_images ()
{
  pushd "$OUTDIR/images"

  for imgfile in `ls *.png`
  do
    if [ -s "$imgfile" ]; then
      convert "$imgfile" -scale 200x "$OUTDIR/images/thumbnails/$imgfile"
    else
      # remove empty files - they usually are a result of a plotting failure
      echo "Removing zero length file:  $imgfile"
      rm -f "$imgfile"
    fi
  done

  popd
}


###############################################################################
#
# generate_latency_html
#
# Generate html page for latency results
#
###############################################################################
generate_latency_html ()
{
  local TITLE="OpenDDS Latency and Jitter Statistics"

  echo '<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">'
  echo '<html>'
  echo '<head>'
  echo "<title>$TITLE</title>"
  echo '<link rel="stylesheet" type="text/css" href="formatting.css" />'
  echo '</head>'
  echo '<body>'

  # page title
  echo '<div class="page_title">'
  echo '<!-- <span class="right_logo"><img class="right_logo" src="http://www.opendds.org/images/openddslogo.jpg" height="110" width="134" alt="OpenDDS Logo" /></span> -->'
  echo "<h1 class=\"page_title\">$TITLE</h1>"
  echo '<div>'
  echo '<br />'

  # table of contents
  echo '<div class="ToC">'
  echo '<h2 class="ToC">Table of Contents</h2>'
  echo '<ul class="ToC">'
  echo '<li class="ToC"><a class="ToC" href="index.html">Back to Test Listing Page</a></li>'
  echo '<li class="ToC"><a class="ToC" href="#summary_graphs">Summary Graphs</a></li>'
  echo '<li class="ToC"><a class="ToC" href="#transport_graphs">Transport Graphs</a></li>'
  if [ -e "$BASEDIR/tests/latency/data/latency-html-tables.txt" ]; then
    echo '<li class="ToC"><a class="ToC" href="#latency_tables">Latency Data</a></li>'
  fi
  if [ -e "$BASEDIR/tests/latency/data/jitter-html-tables.txt" ]; then
    echo '<li class="ToC"><a class="ToC" href="#jitter_tables">Jitter Data</a></li>'
  fi
  if [ -e "$BASEDIR/tests/latency/data/normaljitter-html-tables.txt" ]; then
    echo '<li class="ToC"><a class="ToC" href="#normal_jitter_tables">Normalized Jitter Data</a></li>'
  fi
  echo '<li class="ToC"><a class="ToC" href="#rundate">Run Information</a></li>'
  echo '</ul>'
  echo '<div>'
  echo '<br />'

  # summary graphs
  echo '<div class="graph_fullimage">'
  echo '<h2 class="graph_fullimage">Summary Graphs<a name="summary_graphs">&nbsp;</a></h2>'
  echo '<table class="graph_fullimage">'
  echo '<tr class="graph_fullimage"><td class="graph_fullimage"><a href="images/transport-latency-zoom.png"><img src="images/transport-latency.png" alt="Transport Latency Summary"><br>Transport Latency Summary (click to zoom)</a></td></tr>'
  echo '<tr class="graph_fullimage"><td class="graph_fullimage"><a href="images/transport-jitter-zoom.png"><img src="images/transport-jitter.png" alt="Transport Jitter Summary"><br>Transport Jitter Summary (click to zoom)</a></td></tr>'
  echo '</table>'
  echo '<div>'
  echo '<br />'

  # transport graphs
  echo '<div class="graph_thumbnails">'
  echo '<h2 class="graph_thumbnails">Transport Graphs<a name="transport_graphs">&nbsp;</a></h2>'
  echo '<table class="graph_thumbnails">'
  echo '<colgroup span="4" width="25%"/>'
  echo '<tr class="graph_thumbnails">'
  echo '  <td class="graph_thumbnails"><a href="images/tcp-quantiles.png"><img src="images/thumbnails/tcp-quantiles.png" alt="TCP Latency Quantiles - all sizes"><br>TCP Latency Quantiles - all sizes</a>'
  echo '  <td class="graph_thumbnails"><a href="images/udp-quantiles.png"><img src="images/thumbnails/udp-quantiles.png" alt="UDP Latency Quantiles - all sizes"><br>UDP Latency Quantiles - all sizes</a>'
  echo '  <td class="graph_thumbnails"><a href="images/mbe-quantiles.png"><img src="images/thumbnails/mbe-quantiles.png" alt="Best Effort Multicast Latency Quantiles - all sizes"><br>Best Effort Multicast Latency Quantiles - all sizes</a>'
  echo '  <td class="graph_thumbnails"><a href="images/mrel-quantiles.png"><img src="images/thumbnails/mrel-quantiles.png" alt="Reliable Multicast Latency Quantiles - all sizes"><br>Reliable Multicast Latency Quantiles - all sizes</a>'
  echo '  <td class="graph_thumbnails"><a href="images/rtps-quantiles.png"><img src="images/thumbnails/rtps-quantiles.png" alt="Real-Time Publish-Subscribe Latency Quantiles - all sizes"><br>Real-Time Publish-Subscribe Latency Quantiles - all sizes</a></tr>'
  echo '<tr class="graph_thumbnails">'
  echo '  <td class="graph_thumbnails"><a href="images/tcp-density.png"><img src="images/thumbnails/tcp-density.png" alt="TCP Latency Density - all sizes"><br>TCP Latency Density - all sizes</a>'
  echo '  <td class="graph_thumbnails"><a href="images/udp-density.png"><img src="images/thumbnails/udp-density.png" alt="UDP Latency Density - all sizes"><br>UDP Latency Density - all sizes</a>'
  echo '  <td class="graph_thumbnails"><a href="images/mbe-density.png"><img src="images/thumbnails/mbe-density.png" alt="Best Effort Multicast Latency Density - all sizes"><br>Best Effort Multicast Latency Density - all sizes</a>'
  echo '  <td class="graph_thumbnails"><a href="images/mrel-density.png"><img src="images/thumbnails/mrel-density.png" alt="Reliable Multicast Latency Density - all sizes"><br>Reliable Multicast Latency Density - all sizes</a>'
  echo '  <td class="graph_thumbnails"><a href="images/rtps-density.png"><img src="images/thumbnails/rtps-density.png" alt="Real-Time Publish-Subscribe Latency Density - all sizes"><br>Real-Time Publish-Subscribe Latency Density - all sizes</a></tr>'
  for sz in 50 100 250 500 1000 2500 5000 8000 16000 32000
  do
    echo '<tr class="graph_thumbnails">'
    echo "  <td class=\"graph_thumbnails\"><a href=\"images/latency-tcp-$sz.png\"><img src=\"images/thumbnails/latency-tcp-$sz.png\" alt=\"TCP Latency - $sz bytes\"><br>TCP Latency - $sz bytes</a>"
    echo "  <td class=\"graph_thumbnails\"><a href=\"images/latency-udp-$sz.png\"><img src=\"images/thumbnails/latency-udp-$sz.png\" alt=\"UDP Latency - $sz bytes\"><br>UDP Latency - $sz bytes</a>"
    echo "  <td class=\"graph_thumbnails\"><a href=\"images/latency-mbe-$sz.png\"><img src=\"images/thumbnails/latency-mbe-$sz.png\" alt=\"Best Effort Multicast Latency - $sz bytes\"><br>Best Effort Multicast Latency - $sz bytes</a>"
    echo "  <td class=\"graph_thumbnails\"><a href=\"images/latency-mrel-$sz.png\"><img src=\"images/thumbnails/latency-mrel-$sz.png\" alt=\"Reliable Multicast Latency - $sz bytes\"><br>Reliable Multicast Latency - $sz bytes</a>"
    echo "  <td class=\"graph_thumbnails\"><a href=\"images/latency-rtps-$sz.png\"><img src=\"images/thumbnails/latency-rtps-$sz.png\" alt=\"Real-Time Publish-Subscribe Latency - $sz bytes\"><br>Real-Time Publish-Subscribe Latency - $sz bytes</a></tr>"
  done
  echo '</table>'
  echo '<div>'
  echo '<br />'

  # latency data tables
  if [ -e "$BASEDIR/tests/latency/data/latency-html-tables.txt" ]; then
    cat "$BASEDIR/tests/latency/data/latency-html-tables.txt"
    echo '<br />'
  fi

  # jitter data tables
  if [ -e "$BASEDIR/tests/latency/data/jitter-html-tables.txt" ]; then
    cat "$BASEDIR/tests/latency/data/jitter-html-tables.txt"
    echo '<br />'
  fi

  # normalized jitter data tables
  if [ -e "$BASEDIR/tests/latency/data/normaljitter-html-tables.txt" ]; then
    cat "$BASEDIR/tests/latency/data/normaljitter-html-tables.txt"
    echo '<br />'
  fi

  # run information
  echo "$RUNINFO_HTML"

  echo '</body>'
  echo '</html>'
}


###############################################################################
#
# generate_throughput_html
#
# Generate html page for throughput results
#
###############################################################################
generate_throughput_html ()
{
  local TITLE="Throughput Performance Statistics"

  echo '<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">'
  echo '<html>'
  echo '<head>'
  echo "<title>$TITLE</title>"
  echo '<link rel="stylesheet" type="text/css" href="formatting.css" />'
  echo '</head>'
  echo '<body>'

  # page title
  echo '<div class="page_title">'
  echo '<!-- <span class="right_logo"><img class="right_logo" src="http://www.opendds.org/images/openddslogo.jpg" height="110" width="134" alt="OpenDDS Logo" /></span> -->'
  echo "<h1 class=\"page_title\">$TITLE</h1>"
  echo '<div>'
  echo '<br />'

  # table of contents
  echo '<div class="ToC">'
  echo '<h2 class="ToC">Table of Contents</h2>'
  echo '<ul class="ToC">'
  echo '<li class="ToC"><a class="ToC" href="index.html">Back to Test Listing Page</a></li>'
  echo '<li class="ToC"><a class="ToC" href="#summary_graphs">Summary Graphs</a></li>'
  echo '<li class="ToC"><a class="ToC" href="#transport_graphs">Transport Graphs</a></li>'
  if [ -e "$BASEDIR/tests/thru/data/throughput-html-tables.txt" ]; then
    echo '<li class="ToC"><a class="ToC" href="#bidir_tables">Bidirectional Data</a></li>'
    echo '<li class="ToC"><a class="ToC" href="#pubbound_tables">Publication Bound Data</a></li>'
    echo '<li class="ToC"><a class="ToC" href="#subbound_tables">Subscription Bound Data</a></li>'
  fi
  echo '<li class="ToC"><a class="ToC" href="#rundate">Run Information</a></li>'
  echo '</ul>'
  echo '<div>'
  echo '<br />'

  # summary graphs
  echo '<div class="graph_fullimage">'
  echo '<h2 class="graph_fullimage">Summary Graphs<a name="summary_graphs">&nbsp;</a></h2>'
  echo '<table class="graph_fullimage">'
  echo '<tr class="graph_fullimage"><td class="graph_fullimage"><img src="images/thru-lines.png" alt="Transport Throughput Summary"></td></tr>'
  echo '<tr class="graph_fullimage"><td class="graph_fullimage"><img src="images/thru-bidir.png" alt="Bidirectional Throughput Summary"></td></tr>'
  echo '<tr class="graph_fullimage"><td class="graph_fullimage"><img src="images/thru-pub.png" alt="Publication Bound Throughput Summary"></td></tr>'
  echo '<tr class="graph_fullimage"><td class="graph_fullimage"><img src="images/thru-sub.png" alt="Subscription Bound Throughput Summary"></td></tr>'
  echo '</table>'
  echo '<div>'
  echo '<br />'

  # transport graphs
  echo '<div class="graph_thumbnails">'
  echo '<h2 class="graph_thumbnails">Transport Graphs<a name="transport_graphs">&nbsp;</a></h2>'
  echo '<table class="graph_thumbnails">'
  echo '<colgroup span="4" width="25%"/>'
  echo '<tr class="graph_thumbnails">'
  echo '  <td class="graph_thumbnails"><a href="images/thru-tcp.png"><img src="images/thumbnails/thru-tcp.png" alt="TCP Latency Quantiles - all sizes"><br>TCP Throughput</a>'
  echo '  <td class="graph_thumbnails"><a href="images/thru-udp.png"><img src="images/thumbnails/thru-udp.png" alt="UDP Latency Quantiles - all sizes"><br>UDP Throughput</a>'
  echo '  <td class="graph_thumbnails"><a href="images/thru-mbe.png"><img src="images/thumbnails/thru-mbe.png" alt="Best Effort Multicast Latency Quantiles - all sizes"><br>Best Effort Multicast Throughput</a>'
  echo '  <td class="graph_thumbnails"><a href="images/thru-mrel.png"><img src="images/thumbnails/thru-mrel.png" alt="Reliable Multicast Latency Quantiles - all sizes"><br>Reliable Multicast Throughput</a>'
  echo '  <td class="graph_thumbnails"><a href="images/thru-rtps.png"><img src="images/thumbnails/thru-rtps.png" alt="Real-Time Publish-Subscribe Latency Quantiles - all sizes"><br>Real-Time Publish-Subscribe Throughput</a></tr>'
  echo '</table>'
  echo '<div>'
  echo '<br />'

  # data tables
  if [ -e "$BASEDIR/tests/thru/data/throughput-html-tables.txt" ]; then
    cat "$BASEDIR/tests/thru/data/throughput-html-tables.txt"
    echo '<br />'
  fi

  # run information
  echo "$RUNINFO_HTML"

  echo '</body>'
  echo '</html>'
}


###############################################################################
#
# generate_main_html
#
# Generate main index html page
#
###############################################################################
generate_main_html ()
{
  local TITLE="OpenDDS Performance Statistics"

  echo '<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">'
  echo '<html>'
  echo '<head>'
  echo "<title>$TITLE</title>"
  echo '<link rel="stylesheet" type="text/css" href="formatting.css" />'
  echo '</head>'
  echo '<body>'

  # page title
  echo '<div class="page_title">'
  echo '<!-- <span class="right_logo"><img class="right_logo" src="http://www.opendds.org/images/openddslogo.jpg" height="110" width="134" alt="OpenDDS Logo" /></span> -->'
  echo "<h1 class=\"page_title\">$TITLE</h1>"
  echo '<div>'
  echo '<br />'

  # page summary
  echo '<div class="summary">'
  echo '<p class="summary">These are the results from running the OpenDDS performance test Bench suite.</p>'
  echo '</div>'
  echo '<br />'

  # latency summary
  if [ -e "$OUTDIR/latency.html" ]; then
    echo '<div class="test_summary">'
    echo '<a class="test_summary_header" name="latency" href="latency.html">Latency and Jitter Results</a>'
    echo '<br />'
    echo '<table class="test_summary">'
    echo '<tr class="test_summary">'
    echo '<td class="test_summary"><a href="latency.html"><img src="images/thumbnails/transport-latency.png" alt="Latency per Message graph" /></td>'
    echo '<td class="test_summary"><a href="latency.html"><img src="images/thumbnails/transport-jitter.png" alt="Normalized Jitter per Messsage graph" /></td>'
    echo '</tr>'
    echo '</table>'
    echo '<a class="test_summary_link" href="latency.html">View all Latency and Jitter Results</a>'
    echo '<div>'
    echo '<br />'
  fi

  # throughput summary
  if [ -e "$OUTDIR/throughput.html" ]; then
    echo '<div class="test_summary">'
    echo '<a class="test_summary_header" name="throughput" href="throughput.html">Throughput Results</a>'
    echo '<br />'
    echo '<table class="test_summary">'
    echo '<tr class="test_summary">'
    echo '<td class="test_summary"><a href="throughput.html"><img src="images/thumbnails/thru-lines.png" alt="Throughput of various Testing setups graph" /></td>'
    echo '</tr>'
    echo '</table>'
    echo '<a class="test_summary_link" href="throughput.html">View all Throughput Results</a>'
    echo '<div>'
    echo '<br />'
  fi

  # run information
  echo "$RUNINFO_HTML"

  echo '</body>'
  echo '</html>'
}




OUTDIR="."
BASEDIR="."
DATE=$(date +%Y%m%d)
export PATH=$DDS_ROOT/performance-tests/Bench/bin:$PATH
RUNINFO_HTML=`bash $DDS_ROOT/performance-tests/Bench/bin/gen-run-info-table.pl $DATE`

parse_input $@

create_output_directory

store_result_files

$DDS_ROOT/performance-tests/Bench/bin/generate-test-results.sh "$BASEDIR"

process_latency_test

process_throughput_test

$DDS_ROOT/performance-tests/Bench/bin/plot-test-results.sh "$BASEDIR" "$OUTDIR/images"

process_images

generate_latency_html > "$OUTDIR/latency.html"

generate_throughput_html > "$OUTDIR/throughput.html"

generate_main_html > "$OUTDIR/index.html"
