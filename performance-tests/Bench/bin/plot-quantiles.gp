
set datafile separator whitespace

set terminal push
set terminal png size 1290,770

set grid
set autoscale
unset label
set key outside right

# Parameters for Quantile and Kernel Density Estimation plots
samples=5000
weight=.1

# Latency Quantiles plots
set xlabel "Percentile"
set ylabel "Latency (microseconds)"
set format x "%.1s"
set format y "%.1s%cS"
set xrange [0:100]

set output 'images/tcp-quantiles.png'
set title "TCP - Latency Quantiles per Message Size"
plot 'data/latency-tcp-50.gpd'    index 3 using (100.*column(0)/samples):1:(weight) with lines smooth acsplines title '50',\
     'data/latency-tcp-100.gpd'   index 3 using (100.*column(0)/samples):1:(weight) with lines smooth acsplines title '100',\
     'data/latency-tcp-250.gpd'   index 3 using (100.*column(0)/samples):1:(weight) with lines smooth acsplines title '250',\
     'data/latency-tcp-500.gpd'   index 3 using (100.*column(0)/samples):1:(weight) with lines smooth acsplines title '500',\
     'data/latency-tcp-1000.gpd'  index 3 using (100.*column(0)/samples):1:(weight) with lines smooth acsplines title '1000',\
     'data/latency-tcp-2500.gpd'  index 3 using (100.*column(0)/samples):1:(weight) with lines smooth acsplines title '2500',\
     'data/latency-tcp-5000.gpd'  index 3 using (100.*column(0)/samples):1:(weight) with lines smooth acsplines title '5000',\
     'data/latency-tcp-8000.gpd'  index 3 using (100.*column(0)/samples):1:(weight) with lines smooth acsplines title '8000',\
     'data/latency-tcp-16000.gpd' index 3 using (100.*column(0)/samples):1:(weight) with lines smooth acsplines title '16000',\
     'data/latency-tcp-32000.gpd' index 3 using (100.*column(0)/samples):1:(weight) with lines smooth acsplines title '32000'

set output 'images/udp-quantiles.png'
set title "UDP - Latency Quantiles per Message Size"
plot 'data/latency-udp-50.gpd'    index 3 using (100.*column(0)/samples):1:(weight) with lines smooth acsplines title '50',\
     'data/latency-udp-100.gpd'   index 3 using (100.*column(0)/samples):1:(weight) with lines smooth acsplines title '100',\
     'data/latency-udp-250.gpd'   index 3 using (100.*column(0)/samples):1:(weight) with lines smooth acsplines title '250',\
     'data/latency-udp-500.gpd'   index 3 using (100.*column(0)/samples):1:(weight) with lines smooth acsplines title '500',\
     'data/latency-udp-1000.gpd'  index 3 using (100.*column(0)/samples):1:(weight) with lines smooth acsplines title '1000',\
     'data/latency-udp-2500.gpd'  index 3 using (100.*column(0)/samples):1:(weight) with lines smooth acsplines title '2500',\
     'data/latency-udp-5000.gpd'  index 3 using (100.*column(0)/samples):1:(weight) with lines smooth acsplines title '5000',\
     'data/latency-udp-8000.gpd'  index 3 using (100.*column(0)/samples):1:(weight) with lines smooth acsplines title '8000',\
     'data/latency-udp-16000.gpd' index 3 using (100.*column(0)/samples):1:(weight) with lines smooth acsplines title '16000',\
     'data/latency-udp-32000.gpd' index 3 using (100.*column(0)/samples):1:(weight) with lines smooth acsplines title '32000'

set output 'images/mbe-quantiles.png'
set title "Best Effort Multicast - Latency Quantiles per Message Size"
plot 'data/latency-mbe-50.gpd'    index 3 using (100.*column(0)/samples):1:(weight) with lines smooth acsplines title '50',\
     'data/latency-mbe-100.gpd'   index 3 using (100.*column(0)/samples):1:(weight) with lines smooth acsplines title '100',\
     'data/latency-mbe-250.gpd'   index 3 using (100.*column(0)/samples):1:(weight) with lines smooth acsplines title '250',\
     'data/latency-mbe-500.gpd'   index 3 using (100.*column(0)/samples):1:(weight) with lines smooth acsplines title '500',\
     'data/latency-mbe-1000.gpd'  index 3 using (100.*column(0)/samples):1:(weight) with lines smooth acsplines title '1000',\
     'data/latency-mbe-2500.gpd'  index 3 using (100.*column(0)/samples):1:(weight) with lines smooth acsplines title '2500',\
     'data/latency-mbe-5000.gpd'  index 3 using (100.*column(0)/samples):1:(weight) with lines smooth acsplines title '5000',\
     'data/latency-mbe-8000.gpd'  index 3 using (100.*column(0)/samples):1:(weight) with lines smooth acsplines title '8000',\
     'data/latency-mbe-16000.gpd' index 3 using (100.*column(0)/samples):1:(weight) with lines smooth acsplines title '16000',\
     'data/latency-mbe-32000.gpd' index 3 using (100.*column(0)/samples):1:(weight) with lines smooth acsplines title '32000'

set output 'images/mrel-quantiles.png'
set title "Reliable Multicast - Latency Quantiles per Message Size"
plot 'data/latency-mrel-50.gpd'    index 3 using (100.*column(0)/samples):1:(weight) with lines smooth acsplines title '50',\
     'data/latency-mrel-100.gpd'   index 3 using (100.*column(0)/samples):1:(weight) with lines smooth acsplines title '100',\
     'data/latency-mrel-250.gpd'   index 3 using (100.*column(0)/samples):1:(weight) with lines smooth acsplines title '250',\
     'data/latency-mrel-500.gpd'   index 3 using (100.*column(0)/samples):1:(weight) with lines smooth acsplines title '500',\
     'data/latency-mrel-1000.gpd'  index 3 using (100.*column(0)/samples):1:(weight) with lines smooth acsplines title '1000',\
     'data/latency-mrel-2500.gpd'  index 3 using (100.*column(0)/samples):1:(weight) with lines smooth acsplines title '2500',\
     'data/latency-mrel-5000.gpd'  index 3 using (100.*column(0)/samples):1:(weight) with lines smooth acsplines title '5000',\
     'data/latency-mrel-8000.gpd'  index 3 using (100.*column(0)/samples):1:(weight) with lines smooth acsplines title '8000',\
     'data/latency-mrel-16000.gpd' index 3 using (100.*column(0)/samples):1:(weight) with lines smooth acsplines title '16000',\
     'data/latency-mrel-32000.gpd' index 3 using (100.*column(0)/samples):1:(weight) with lines smooth acsplines title '32000'

set output
set terminal pop

# vim: filetype=gnuplot tw=200

