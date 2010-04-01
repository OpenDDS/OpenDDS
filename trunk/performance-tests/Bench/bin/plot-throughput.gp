# Call parameters:
#   $0 - datafilename
#   $1 - outputfilename

set terminal push
set terminal png size 1290,770

set datafile separator ","
set grid
set autoscale
set key top left box
set title  "Throughput per Message Size"
set xlabel "Message Size (1000 bytes)"
set ylabel "Throughput (1000000 bytes per second)"
set format x "%.1s%c"
set format y "%.1s%c"

set output '$1-all-size.png'
plot '$0'\
     index 2 using (column(2)/1000):(column(4)/1000000) t "TCP"                   with linespoints,\
  '' index 3 using (column(2)/1000):(column(4)/1000000) t "UDP"                   with linespoints,\
  '' index 0 using (column(2)/1000):(column(4)/1000000) t "multicast/best effort" with linespoints,\
  '' index 1 using (column(2)/1000):(column(4)/1000000) t "multicast/reliable"    with linespoints

set xrange [10:60]
set output '$1-all-size-zoom.png'
replot



set autoscale
set title  "Throughput per Message Rate"
set xlabel "Message Rate (1000 messages per second)"

set output '$1-all-rate.png'
plot '$0'\
     index 2 using (column(3)/1000):(column(4)/1000000) t "TCP"                   with linespoints,\
  '' index 3 using (column(3)/1000):(column(4)/1000000) t "UDP"                   with linespoints,\
  '' index 0 using (column(3)/1000):(column(4)/1000000) t "multicast/best effort" with linespoints,\
  '' index 1 using (column(3)/1000):(column(4)/1000000) t "multicast/reliable"    with linespoints

set xrange [1:6]
set output '$1-all-rate-zoom.png'
replot



set autoscale
set title  "Throughput per Data Volume"
set xlabel "Data Volume (1000000 bytes per second)"

set output '$1-all-volume.png'
plot '$0'\
     index 2 using (column(2)*column(3)/1000000):(column(4)/1000000) t "TCP"                   with linespoints,\
  '' index 3 using (column(2)*column(3)/1000000):(column(4)/1000000) t "UDP"                   with linespoints,\
  '' index 0 using (column(2)*column(3)/1000000):(column(4)/1000000) t "multicast/best effort" with linespoints,\
  '' index 1 using (column(2)*column(3)/1000000):(column(4)/1000000) t "multicast/reliable"    with linespoints

set xrange [1:250]
set output '$1-all-volume-zoom.png'
replot





set output
set terminal pop
