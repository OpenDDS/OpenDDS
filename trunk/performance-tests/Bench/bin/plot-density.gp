
set datafile separator whitespace

set terminal push
set terminal png size 1290,770

set grid
set autoscale
unset label
set key outside right

# Parameters for Quantile and Kernel Density Estimation plots
samples=5000
hops=2.
bandwidth=.00001

# Latency Kernel Density Estimation plots
set autoscale
set xlabel "Latency (microseconds)"
set ylabel "Samples"
set format x "%.1s%cS"
set format y "%.0f%%"
set xtics out nomirror rotate by 15 offset -5,-1

set output 'images/tcp-density.png'
set title "TCP - Latency Density Plot per Message Size"
plot 'data/latency-tcp-50.gpd'    index 0 using ($1/hops):(100.*bandwidth/samples):(bandwidth) smooth kdensity t '50',\
     'data/latency-tcp-100.gpd'   index 0 using ($1/hops):(100.*bandwidth/samples):(bandwidth) smooth kdensity t '100',\
     'data/latency-tcp-250.gpd'   index 0 using ($1/hops):(100.*bandwidth/samples):(bandwidth) smooth kdensity t '250',\
     'data/latency-tcp-500.gpd'   index 0 using ($1/hops):(100.*bandwidth/samples):(bandwidth) smooth kdensity t '500',\
     'data/latency-tcp-1000.gpd'  index 0 using ($1/hops):(100.*bandwidth/samples):(bandwidth) smooth kdensity t '1000',\
     'data/latency-tcp-2500.gpd'  index 0 using ($1/hops):(100.*bandwidth/samples):(bandwidth) smooth kdensity t '2500',\
     'data/latency-tcp-5000.gpd'  index 0 using ($1/hops):(100.*bandwidth/samples):(bandwidth) smooth kdensity t '5000',\
     'data/latency-tcp-8000.gpd'  index 0 using ($1/hops):(100.*bandwidth/samples):(bandwidth) smooth kdensity t '8000',\
     'data/latency-tcp-16000.gpd' index 0 using ($1/hops):(100.*bandwidth/samples):(bandwidth) smooth kdensity t '16000',\
     'data/latency-tcp-32000.gpd' index 0 using ($1/hops):(100.*bandwidth/samples):(bandwidth) smooth kdensity t '32000'

set output 'images/udp-density.png'
set title "UDP - Latency Density Plot per Message Size"
plot 'data/latency-udp-50.gpd'    index 0 using ($1/hops):(100.*bandwidth/samples):(bandwidth) smooth kdensity t '50',\
     'data/latency-udp-100.gpd'   index 0 using ($1/hops):(100.*bandwidth/samples):(bandwidth) smooth kdensity t '100',\
     'data/latency-udp-250.gpd'   index 0 using ($1/hops):(100.*bandwidth/samples):(bandwidth) smooth kdensity t '250',\
     'data/latency-udp-500.gpd'   index 0 using ($1/hops):(100.*bandwidth/samples):(bandwidth) smooth kdensity t '500',\
     'data/latency-udp-1000.gpd'  index 0 using ($1/hops):(100.*bandwidth/samples):(bandwidth) smooth kdensity t '1000',\
     'data/latency-udp-2500.gpd'  index 0 using ($1/hops):(100.*bandwidth/samples):(bandwidth) smooth kdensity t '2500',\
     'data/latency-udp-5000.gpd'  index 0 using ($1/hops):(100.*bandwidth/samples):(bandwidth) smooth kdensity t '5000',\
     'data/latency-udp-8000.gpd'  index 0 using ($1/hops):(100.*bandwidth/samples):(bandwidth) smooth kdensity t '8000',\
     'data/latency-udp-16000.gpd' index 0 using ($1/hops):(100.*bandwidth/samples):(bandwidth) smooth kdensity t '16000',\
     'data/latency-udp-32000.gpd' index 0 using ($1/hops):(100.*bandwidth/samples):(bandwidth) smooth kdensity t '32000'

set output 'images/mbe-density.png'
set title "Best Effort Multicast - Latency Density Plot per Message Size"
plot 'data/latency-mbe-50.gpd'    index 0 using ($1/hops):(100.*bandwidth/samples):(bandwidth) smooth kdensity t '50',\
     'data/latency-mbe-100.gpd'   index 0 using ($1/hops):(100.*bandwidth/samples):(bandwidth) smooth kdensity t '100',\
     'data/latency-mbe-250.gpd'   index 0 using ($1/hops):(100.*bandwidth/samples):(bandwidth) smooth kdensity t '250',\
     'data/latency-mbe-500.gpd'   index 0 using ($1/hops):(100.*bandwidth/samples):(bandwidth) smooth kdensity t '500',\
     'data/latency-mbe-1000.gpd'  index 0 using ($1/hops):(100.*bandwidth/samples):(bandwidth) smooth kdensity t '1000',\
     'data/latency-mbe-2500.gpd'  index 0 using ($1/hops):(100.*bandwidth/samples):(bandwidth) smooth kdensity t '2500',\
     'data/latency-mbe-5000.gpd'  index 0 using ($1/hops):(100.*bandwidth/samples):(bandwidth) smooth kdensity t '5000',\
     'data/latency-mbe-8000.gpd'  index 0 using ($1/hops):(100.*bandwidth/samples):(bandwidth) smooth kdensity t '8000',\
     'data/latency-mbe-16000.gpd' index 0 using ($1/hops):(100.*bandwidth/samples):(bandwidth) smooth kdensity t '16000',\
     'data/latency-mbe-32000.gpd' index 0 using ($1/hops):(100.*bandwidth/samples):(bandwidth) smooth kdensity t '32000'

set output 'images/mrel-density.png'
set title "Reliable Multicast - Latency Density Plot per Message Size"
plot 'data/latency-mrel-50.gpd'    index 0 using ($1/hops):(100.*bandwidth/samples):(bandwidth) smooth kdensity t '50',\
     'data/latency-mrel-100.gpd'   index 0 using ($1/hops):(100.*bandwidth/samples):(bandwidth) smooth kdensity t '100',\
     'data/latency-mrel-250.gpd'   index 0 using ($1/hops):(100.*bandwidth/samples):(bandwidth) smooth kdensity t '250',\
     'data/latency-mrel-500.gpd'   index 0 using ($1/hops):(100.*bandwidth/samples):(bandwidth) smooth kdensity t '500',\
     'data/latency-mrel-1000.gpd'  index 0 using ($1/hops):(100.*bandwidth/samples):(bandwidth) smooth kdensity t '1000',\
     'data/latency-mrel-2500.gpd'  index 0 using ($1/hops):(100.*bandwidth/samples):(bandwidth) smooth kdensity t '2500',\
     'data/latency-mrel-5000.gpd'  index 0 using ($1/hops):(100.*bandwidth/samples):(bandwidth) smooth kdensity t '5000',\
     'data/latency-mrel-8000.gpd'  index 0 using ($1/hops):(100.*bandwidth/samples):(bandwidth) smooth kdensity t '8000',\
     'data/latency-mrel-16000.gpd' index 0 using ($1/hops):(100.*bandwidth/samples):(bandwidth) smooth kdensity t '16000',\
     'data/latency-mrel-32000.gpd' index 0 using ($1/hops):(100.*bandwidth/samples):(bandwidth) smooth kdensity t '32000'

set output
set terminal pop

# vim: filetype=gnuplot tw=200

