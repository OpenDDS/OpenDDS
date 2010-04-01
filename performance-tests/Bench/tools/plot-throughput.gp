
set loadpath "`echo $DDS_ROOT/performance-tests/Bench/bin`"
show loadpath

print 'Plotting summary charts'
call 'plot-throughput.gp' 'data/throughput.csv' 'images/throughput'

