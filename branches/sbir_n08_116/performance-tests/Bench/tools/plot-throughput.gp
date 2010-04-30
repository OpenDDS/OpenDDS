
set loadpath "`echo $DDS_ROOT/performance-tests/Bench/bin`"
show loadpath

print 'Plotting test format charts'
call 'plot-throughput-testformats.gp' 'data/throughput.csv' 'images'

print 'Plotting transport charts'
call 'plot-throughput-transports.gp' 'data/throughput.csv' 'images'
