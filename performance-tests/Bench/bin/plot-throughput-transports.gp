# Call parameters:
#   $0 - datafile name
#   $1 - output directory

set datafile separator whitespace
set grid
set autoscale
set key below

set format x2 "%.1s%c"
set format x  "%.1s%c"
set format y  "%.1s%cbps"

# 100Mbps network
capacity=100000000

set xlabel 'Message Size (bytes)'
set x2label 'Message Rate (samples per second)'
set ylabel 'Throughput'
set xtics out nomirror rotate by 15 offset -5,-1
set x2tics out rotate by 15
set yrange [0:1.5*capacity]

# TCP charts
set title 'TCP Throughput'
plot capacity with lines t 'Network Capacity',\
     '$0' index 6 using 1:(column(1)*column(2)*8.):(0.1) with lines smooth acspline t 'Test Rate',\
     ''             index 6 using 1:3:(0.1) with lines smooth acspline t 'Bidirectional: rate and size vary',\
     ''             index 6 using 1:3 with points t 'data points:',\
     ''             index 7 using 1:3:(0.1) with lines smooth acspline t 'Bidirectional: rate at 1,000',\
     ''             index 7 using 1:3 with points t 'data points:',\
     ''             index 8 using 2:3:(0.1) with lines smooth acspline axes x2y1 t 'Bidirectional: size at 10,000',\
     ''             index 8 using 2:3 with points axes x2y1 t 'data points:'

set terminal push
set terminal png size 1000,750
set output '$1/thru-tcp.png'
replot
set output
set terminal pop


# UDP charts
set title 'UDP Throughput'
plot capacity with lines t 'Network Capacity',\
     '$0' index 9 using 1:(column(1)*column(2)*8.):(0.1) with lines smooth acspline t 'Test Rate',\
     ''             index 9 using 1:3:(0.1) with lines smooth acspline t 'Bidirectional: rate and size vary',\
     ''             index 9 using 1:3 with points t 'data points:',\
     ''             index 10 using 1:3:(0.1) with lines smooth acspline t 'Bidirectional: rate at 1,000',\
     ''             index 10 using 1:3 with points t 'data points:',\
     ''             index 11 using 2:3:(0.1) with lines smooth acspline axes x2y1 t 'Bidirectional: size at 10,000',\
     ''             index 11 using 2:3 with points axes x2y1 t 'data points:'

set terminal push
set terminal png size 1000,750
set output '$1/thru-udp.png'
replot
set output
set terminal pop


# Best Effort Multicast charts
set title 'Best Effort Multicast Throughput'
plot capacity with lines t 'Network Capacity',\
     '$0' index 0 using 1:(column(1)*column(2)*8.):(0.1) with lines smooth acspline t 'Test Rate',\
     ''             index 0 using 1:3:(0.1) with lines smooth acspline t 'Bidirectional: rate and size vary',\
     ''             index 0 using 1:3 with points t 'data points:',\
     ''             index 1 using 1:3:(0.1) with lines smooth acspline t 'Bidirectional: rate at 1,000',\
     ''             index 1 using 1:3 with points t 'data points:',\
     ''             index 2 using 2:3:(0.1) with lines smooth acspline axes x2y1 t 'Bidirectional: size at 10,000',\
     ''             index 2 using 2:3 with points axes x2y1 t 'data points:'

set terminal push
set terminal png size 1000,750
set output '$1/thru-bemc.png'
replot
set output
set terminal pop


# Reliable Multicast charts
set title 'Reliable Multicast Throughput'
plot capacity with lines t 'Network Capacity',\
     '$0' index 3 using 1:(column(1)*column(2)*8.):(0.1) with lines smooth acspline t 'Test Rate',\
     ''             index 3 using 1:3:(0.1) with lines smooth acspline t 'Bidirectional: rate and size vary',\
     ''             index 3 using 1:3 with points t 'data points:',\
     ''             index 4 using 1:3:(0.1) with lines smooth acspline t 'Bidirectional: rate at 1,000',\
     ''             index 4 using 1:3 with points t 'data points:',\
     ''             index 5 using 2:3:(0.1) with lines smooth acspline axes x2y1 t 'Bidirectional: size at 10,000',\
     ''             index 5 using 2:3 with points axes x2y1 t 'data points:'

set terminal push
set terminal png size 1000,750
set output '$1/thru-rmc.png'
replot
set output
set terminal pop

