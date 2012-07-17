#!/bin/bash -vx

export BENCHBASE=$DDS_ROOT/performance-tests/Bench
export TESTBASE=$BENCHBASE/tests/shared
export REDUCECMD="$BENCHBASE/bin/reduce-shared-data.pl "

export TRANSPORT_TCP=$BENCHBASE/tests/shared/transport-tcp.ini
export TRANSPORT_UDP=$BENCHBASE/tests/shared/transport-udp.ini
export TRANSPORT_MCAST_BE=$BENCHBASE/tests/shared/transport-mcast-be.ini
export TRANSPORT_MCAST_REL=$BENCHBASE/tests/shared/transport-mcast-rel.ini
export TRANSPORT_RTPS=$BENCHBASE/tests/shared/transport-rtps.ini

mkdir -p data

$REDUCECMD $TESTBASE/run/1-1/shmem-latency-s1.data > data/1-1-shmem-rel-s1.gpd
$REDUCECMD $TESTBASE/run/2-1/shmem-latency-s1.data > data/2-1-shmem-rel-s1.gpd
$REDUCECMD $TESTBASE/run/4-1/shmem-latency-s1.data > data/4-1-shmem-rel-s1.gpd
$REDUCECMD $TESTBASE/run/8-1/shmem-latency-s1.data > data/8-1-shmem-rel-s1.gpd
$REDUCECMD $TESTBASE/run/16-1/shmem-latency-s1.data > data/16-1-shmem-rel-s1.gpd

$REDUCECMD $TESTBASE/run/1-1/tcp-latency-s1.data > data/1-1-tcp-rel-s1.gpd
$REDUCECMD $TESTBASE/run/2-1/tcp-latency-s1.data > data/2-1-tcp-rel-s1.gpd
$REDUCECMD $TESTBASE/run/4-1/tcp-latency-s1.data > data/4-1-tcp-rel-s1.gpd
$REDUCECMD $TESTBASE/run/8-1/tcp-latency-s1.data > data/8-1-tcp-rel-s1.gpd
$REDUCECMD $TESTBASE/run/16-1/tcp-latency-s1.data > data/16-1-tcp-rel-s1.gpd

$REDUCECMD $TESTBASE/run/1-1/udp-latency-s1.data > data/1-1-udp-be-s1.gpd
$REDUCECMD $TESTBASE/run/2-1/udp-latency-s1.data > data/1-2-udp-be-s1.gpd
$REDUCECMD $TESTBASE/run/4-1/udp-latency-s1.data > data/1-4-udp-be-s1.gpd
$REDUCECMD $TESTBASE/run/8-1/udp-latency-s1.data > data/1-8-udp-be-s1.gpd
$REDUCECMD $TESTBASE/run/16-1/udp-latency-s1.data > data/1-16-udp-be-s1.gpd

$REDUCECMD $TESTBASE/run/1-1/mcast-rel-latency-s1.data > data/1-1-mcast-rel-s1.gpd
$REDUCECMD $TESTBASE/run/2-1/mcast-rel-latency-s1.data > data/2-1-mcast-rel-s1.gpd
$REDUCECMD $TESTBASE/run/4-1/mcast-rel-latency-s1.data > data/4-1-mcast-rel-s1.gpd
$REDUCECMD $TESTBASE/run/8-1/mcast-rel-latency-s1.data > data/8-1-mcast-rel-s1.gpd
$REDUCECMD $TESTBASE/run/16-1/mcast-rel-latency-s1.data > data/16-1-mcast-rel-s1.gpd

$REDUCECMD $TESTBASE/run/1-1/mcast-be-latency-s1.data > data/1-1-mcast-be-s1.gpd
$REDUCECMD $TESTBASE/run/2-1/mcast-be-latency-s1.data > data/2-1-mcast-be-s1.gpd
$REDUCECMD $TESTBASE/run/4-1/mcast-be-latency-s1.data > data/4-1-mcast-be-s1.gpd
$REDUCECMD $TESTBASE/run/8-1/mcast-be-latency-s1.data > data/8-1-mcast-be-s1.gpd
$REDUCECMD $TESTBASE/run/16-1/mcast-be-latency-s1.data > data/16-1-mcast-be-s1.gpd

$REDUCECMD $TESTBASE/run/1-1/rtps-latency-s1.data > data/1-1-rtps-rel-s1.gpd
$REDUCECMD $TESTBASE/run/2-1/rtps-latency-s1.data > data/2-1-rtps-rel-s1.gpd
$REDUCECMD $TESTBASE/run/4-1/rtps-latency-s1.data > data/4-1-rtps-rel-s1.gpd
$REDUCECMD $TESTBASE/run/8-1/rtps-latency-s1.data > data/8-1-rtps-rel-s1.gpd
$REDUCECMD $TESTBASE/run/16-1/rtps-latency-s1.data > data/16-1-rtps-rel-s1.gpd

$REDUCECMD $TESTBASE/run/1-2/shmem-latency-s1.data > data/1-2-shmem-rel-s1.gpd
$REDUCECMD $TESTBASE/run/1-2/shmem-latency-s2.data > data/1-2-shmem-rel-s2.gpd
$REDUCECMD $TESTBASE/run/1-4/shmem-latency-s1.data > data/1-4-shmem-rel-s1.gpd
$REDUCECMD $TESTBASE/run/1-4/shmem-latency-s2.data > data/1-4-shmem-rel-s2.gpd
$REDUCECMD $TESTBASE/run/1-4/shmem-latency-s3.data > data/1-4-shmem-rel-s3.gpd
$REDUCECMD $TESTBASE/run/1-4/shmem-latency-s4.data > data/1-4-shmem-rel-s4.gpd
$REDUCECMD $TESTBASE/run/1-8/shmem-latency-s1.data > data/1-8-shmem-rel-s1.gpd
$REDUCECMD $TESTBASE/run/1-8/shmem-latency-s2.data > data/1-8-shmem-rel-s2.gpd
$REDUCECMD $TESTBASE/run/1-8/shmem-latency-s3.data > data/1-8-shmem-rel-s3.gpd
$REDUCECMD $TESTBASE/run/1-8/shmem-latency-s4.data > data/1-8-shmem-rel-s4.gpd
$REDUCECMD $TESTBASE/run/1-8/shmem-latency-s5.data > data/1-8-shmem-rel-s5.gpd
$REDUCECMD $TESTBASE/run/1-8/shmem-latency-s6.data > data/1-8-shmem-rel-s6.gpd
$REDUCECMD $TESTBASE/run/1-8/shmem-latency-s7.data > data/1-8-shmem-rel-s7.gpd
$REDUCECMD $TESTBASE/run/1-8/shmem-latency-s8.data > data/1-8-shmem-rel-s8.gpd
$REDUCECMD $TESTBASE/run/1-16/shmem-latency-s1.data > data/1-16-shmem-rel-s1.gpd
$REDUCECMD $TESTBASE/run/1-16/shmem-latency-s2.data > data/1-16-shmem-rel-s2.gpd
$REDUCECMD $TESTBASE/run/1-16/shmem-latency-s3.data > data/1-16-shmem-rel-s3.gpd
$REDUCECMD $TESTBASE/run/1-16/shmem-latency-s4.data > data/1-16-shmem-rel-s4.gpd
$REDUCECMD $TESTBASE/run/1-16/shmem-latency-s5.data > data/1-16-shmem-rel-s5.gpd
$REDUCECMD $TESTBASE/run/1-16/shmem-latency-s6.data > data/1-16-shmem-rel-s6.gpd
$REDUCECMD $TESTBASE/run/1-16/shmem-latency-s7.data > data/1-16-shmem-rel-s7.gpd
$REDUCECMD $TESTBASE/run/1-16/shmem-latency-s8.data > data/1-16-shmem-rel-s8.gpd
$REDUCECMD $TESTBASE/run/1-16/shmem-latency-s9.data > data/1-16-shmem-rel-s9.gpd
$REDUCECMD $TESTBASE/run/1-16/shmem-latency-s10.data > data/1-16-shmem-rel-s10.gpd
$REDUCECMD $TESTBASE/run/1-16/shmem-latency-s11.data > data/1-16-shmem-rel-s11.gpd
$REDUCECMD $TESTBASE/run/1-16/shmem-latency-s12.data > data/1-16-shmem-rel-s12.gpd
$REDUCECMD $TESTBASE/run/1-16/shmem-latency-s13.data > data/1-16-shmem-rel-s13.gpd
$REDUCECMD $TESTBASE/run/1-16/shmem-latency-s14.data > data/1-16-shmem-rel-s14.gpd
$REDUCECMD $TESTBASE/run/1-16/shmem-latency-s15.data > data/1-16-shmem-rel-s15.gpd
$REDUCECMD $TESTBASE/run/1-16/shmem-latency-s16.data > data/1-16-shmem-rel-s16.gpd

$REDUCECMD $TESTBASE/run/1-2/tcp-latency-s1.data > data/1-2-tcp-rel-s1.gpd
$REDUCECMD $TESTBASE/run/1-2/tcp-latency-s2.data > data/1-2-tcp-rel-s2.gpd
$REDUCECMD $TESTBASE/run/1-4/tcp-latency-s1.data > data/1-4-tcp-rel-s1.gpd
$REDUCECMD $TESTBASE/run/1-4/tcp-latency-s2.data > data/1-4-tcp-rel-s2.gpd
$REDUCECMD $TESTBASE/run/1-4/tcp-latency-s3.data > data/1-4-tcp-rel-s3.gpd
$REDUCECMD $TESTBASE/run/1-4/tcp-latency-s4.data > data/1-4-tcp-rel-s4.gpd
$REDUCECMD $TESTBASE/run/1-8/tcp-latency-s1.data > data/1-8-tcp-rel-s1.gpd
$REDUCECMD $TESTBASE/run/1-8/tcp-latency-s2.data > data/1-8-tcp-rel-s2.gpd
$REDUCECMD $TESTBASE/run/1-8/tcp-latency-s3.data > data/1-8-tcp-rel-s3.gpd
$REDUCECMD $TESTBASE/run/1-8/tcp-latency-s4.data > data/1-8-tcp-rel-s4.gpd
$REDUCECMD $TESTBASE/run/1-8/tcp-latency-s5.data > data/1-8-tcp-rel-s5.gpd
$REDUCECMD $TESTBASE/run/1-8/tcp-latency-s6.data > data/1-8-tcp-rel-s6.gpd
$REDUCECMD $TESTBASE/run/1-8/tcp-latency-s7.data > data/1-8-tcp-rel-s7.gpd
$REDUCECMD $TESTBASE/run/1-8/tcp-latency-s8.data > data/1-8-tcp-rel-s8.gpd
$REDUCECMD $TESTBASE/run/1-16/tcp-latency-s1.data > data/1-16-tcp-rel-s1.gpd
$REDUCECMD $TESTBASE/run/1-16/tcp-latency-s2.data > data/1-16-tcp-rel-s2.gpd
$REDUCECMD $TESTBASE/run/1-16/tcp-latency-s3.data > data/1-16-tcp-rel-s3.gpd
$REDUCECMD $TESTBASE/run/1-16/tcp-latency-s4.data > data/1-16-tcp-rel-s4.gpd
$REDUCECMD $TESTBASE/run/1-16/tcp-latency-s5.data > data/1-16-tcp-rel-s5.gpd
$REDUCECMD $TESTBASE/run/1-16/tcp-latency-s6.data > data/1-16-tcp-rel-s6.gpd
$REDUCECMD $TESTBASE/run/1-16/tcp-latency-s7.data > data/1-16-tcp-rel-s7.gpd
$REDUCECMD $TESTBASE/run/1-16/tcp-latency-s8.data > data/1-16-tcp-rel-s8.gpd
$REDUCECMD $TESTBASE/run/1-16/tcp-latency-s9.data > data/1-16-tcp-rel-s9.gpd
$REDUCECMD $TESTBASE/run/1-16/tcp-latency-s10.data > data/1-16-tcp-rel-s10.gpd
$REDUCECMD $TESTBASE/run/1-16/tcp-latency-s11.data > data/1-16-tcp-rel-s11.gpd
$REDUCECMD $TESTBASE/run/1-16/tcp-latency-s12.data > data/1-16-tcp-rel-s12.gpd
$REDUCECMD $TESTBASE/run/1-16/tcp-latency-s13.data > data/1-16-tcp-rel-s13.gpd
$REDUCECMD $TESTBASE/run/1-16/tcp-latency-s14.data > data/1-16-tcp-rel-s14.gpd
$REDUCECMD $TESTBASE/run/1-16/tcp-latency-s15.data > data/1-16-tcp-rel-s15.gpd
$REDUCECMD $TESTBASE/run/1-16/tcp-latency-s16.data > data/1-16-tcp-rel-s16.gpd

$REDUCECMD $TESTBASE/run/1-2/udp-latency-s1.data > data/1-2-udp-be-s1.gpd
$REDUCECMD $TESTBASE/run/1-2/udp-latency-s2.data > data/1-2-udp-be-s2.gpd
$REDUCECMD $TESTBASE/run/1-4/udp-latency-s1.data > data/1-4-udp-be-s1.gpd
$REDUCECMD $TESTBASE/run/1-4/udp-latency-s2.data > data/1-4-udp-be-s2.gpd
$REDUCECMD $TESTBASE/run/1-4/udp-latency-s3.data > data/1-4-udp-be-s3.gpd
$REDUCECMD $TESTBASE/run/1-4/udp-latency-s4.data > data/1-4-udp-be-s4.gpd
$REDUCECMD $TESTBASE/run/1-8/udp-latency-s1.data > data/1-8-udp-be-s1.gpd
$REDUCECMD $TESTBASE/run/1-8/udp-latency-s2.data > data/1-8-udp-be-s2.gpd
$REDUCECMD $TESTBASE/run/1-8/udp-latency-s3.data > data/1-8-udp-be-s3.gpd
$REDUCECMD $TESTBASE/run/1-8/udp-latency-s4.data > data/1-8-udp-be-s4.gpd
$REDUCECMD $TESTBASE/run/1-8/udp-latency-s5.data > data/1-8-udp-be-s5.gpd
$REDUCECMD $TESTBASE/run/1-8/udp-latency-s6.data > data/1-8-udp-be-s6.gpd
$REDUCECMD $TESTBASE/run/1-8/udp-latency-s7.data > data/1-8-udp-be-s7.gpd
$REDUCECMD $TESTBASE/run/1-8/udp-latency-s8.data > data/1-8-udp-be-s8.gpd
$REDUCECMD $TESTBASE/run/1-16/udp-latency-s1.data > data/1-16-udp-be-s1.gpd
$REDUCECMD $TESTBASE/run/1-16/udp-latency-s2.data > data/1-16-udp-be-s2.gpd
$REDUCECMD $TESTBASE/run/1-16/udp-latency-s3.data > data/1-16-udp-be-s3.gpd
$REDUCECMD $TESTBASE/run/1-16/udp-latency-s4.data > data/1-16-udp-be-s4.gpd
$REDUCECMD $TESTBASE/run/1-16/udp-latency-s5.data > data/1-16-udp-be-s5.gpd
$REDUCECMD $TESTBASE/run/1-16/udp-latency-s6.data > data/1-16-udp-be-s6.gpd
$REDUCECMD $TESTBASE/run/1-16/udp-latency-s7.data > data/1-16-udp-be-s7.gpd
$REDUCECMD $TESTBASE/run/1-16/udp-latency-s8.data > data/1-16-udp-be-s8.gpd
$REDUCECMD $TESTBASE/run/1-16/udp-latency-s9.data > data/1-16-udp-be-s9.gpd
$REDUCECMD $TESTBASE/run/1-16/udp-latency-s10.data > data/1-16-udp-be-s10.gpd
$REDUCECMD $TESTBASE/run/1-16/udp-latency-s11.data > data/1-16-udp-be-s11.gpd
$REDUCECMD $TESTBASE/run/1-16/udp-latency-s12.data > data/1-16-udp-be-s12.gpd
$REDUCECMD $TESTBASE/run/1-16/udp-latency-s13.data > data/1-16-udp-be-s13.gpd
$REDUCECMD $TESTBASE/run/1-16/udp-latency-s14.data > data/1-16-udp-be-s14.gpd
$REDUCECMD $TESTBASE/run/1-16/udp-latency-s15.data > data/1-16-udp-be-s15.gpd
$REDUCECMD $TESTBASE/run/1-16/udp-latency-s16.data > data/1-16-udp-be-s16.gpd

$REDUCECMD $TESTBASE/run/1-2/mcast-rel-latency-s1.data > data/1-2-mcast-rel-s1.gpd
$REDUCECMD $TESTBASE/run/1-2/mcast-rel-latency-s2.data > data/1-2-mcast-rel-s2.gpd
$REDUCECMD $TESTBASE/run/1-4/mcast-rel-latency-s1.data > data/1-4-mcast-rel-s1.gpd
$REDUCECMD $TESTBASE/run/1-4/mcast-rel-latency-s2.data > data/1-4-mcast-rel-s2.gpd
$REDUCECMD $TESTBASE/run/1-4/mcast-rel-latency-s3.data > data/1-4-mcast-rel-s3.gpd
$REDUCECMD $TESTBASE/run/1-4/mcast-rel-latency-s4.data > data/1-4-mcast-rel-s4.gpd
$REDUCECMD $TESTBASE/run/1-8/mcast-rel-latency-s1.data > data/1-8-mcast-rel-s1.gpd
$REDUCECMD $TESTBASE/run/1-8/mcast-rel-latency-s2.data > data/1-8-mcast-rel-s2.gpd
$REDUCECMD $TESTBASE/run/1-8/mcast-rel-latency-s3.data > data/1-8-mcast-rel-s3.gpd
$REDUCECMD $TESTBASE/run/1-8/mcast-rel-latency-s4.data > data/1-8-mcast-rel-s4.gpd
$REDUCECMD $TESTBASE/run/1-8/mcast-rel-latency-s5.data > data/1-8-mcast-rel-s5.gpd
$REDUCECMD $TESTBASE/run/1-8/mcast-rel-latency-s6.data > data/1-8-mcast-rel-s6.gpd
$REDUCECMD $TESTBASE/run/1-8/mcast-rel-latency-s7.data > data/1-8-mcast-rel-s7.gpd
$REDUCECMD $TESTBASE/run/1-8/mcast-rel-latency-s8.data > data/1-8-mcast-rel-s8.gpd
$REDUCECMD $TESTBASE/run/1-16/mcast-rel-latency-s1.data > data/1-16-mcast-rel-s1.gpd
$REDUCECMD $TESTBASE/run/1-16/mcast-rel-latency-s2.data > data/1-16-mcast-rel-s2.gpd
$REDUCECMD $TESTBASE/run/1-16/mcast-rel-latency-s3.data > data/1-16-mcast-rel-s3.gpd
$REDUCECMD $TESTBASE/run/1-16/mcast-rel-latency-s4.data > data/1-16-mcast-rel-s4.gpd
$REDUCECMD $TESTBASE/run/1-16/mcast-rel-latency-s5.data > data/1-16-mcast-rel-s5.gpd
$REDUCECMD $TESTBASE/run/1-16/mcast-rel-latency-s6.data > data/1-16-mcast-rel-s6.gpd
$REDUCECMD $TESTBASE/run/1-16/mcast-rel-latency-s7.data > data/1-16-mcast-rel-s7.gpd
$REDUCECMD $TESTBASE/run/1-16/mcast-rel-latency-s8.data > data/1-16-mcast-rel-s8.gpd
$REDUCECMD $TESTBASE/run/1-16/mcast-rel-latency-s9.data > data/1-16-mcast-rel-s9.gpd
$REDUCECMD $TESTBASE/run/1-16/mcast-rel-latency-s10.data > data/1-16-mcast-rel-s10.gpd
$REDUCECMD $TESTBASE/run/1-16/mcast-rel-latency-s11.data > data/1-16-mcast-rel-s11.gpd
$REDUCECMD $TESTBASE/run/1-16/mcast-rel-latency-s12.data > data/1-16-mcast-rel-s12.gpd
$REDUCECMD $TESTBASE/run/1-16/mcast-rel-latency-s13.data > data/1-16-mcast-rel-s13.gpd
$REDUCECMD $TESTBASE/run/1-16/mcast-rel-latency-s14.data > data/1-16-mcast-rel-s14.gpd
$REDUCECMD $TESTBASE/run/1-16/mcast-rel-latency-s15.data > data/1-16-mcast-rel-s15.gpd
$REDUCECMD $TESTBASE/run/1-16/mcast-rel-latency-s16.data > data/1-16-mcast-rel-s16.gpd

$REDUCECMD $TESTBASE/run/1-2/mcast-be-latency-s1.data > data/1-2-mcast-be-s1.gpd
$REDUCECMD $TESTBASE/run/1-2/mcast-be-latency-s2.data > data/1-2-mcast-be-s2.gpd
$REDUCECMD $TESTBASE/run/1-4/mcast-be-latency-s1.data > data/1-4-mcast-be-s1.gpd
$REDUCECMD $TESTBASE/run/1-4/mcast-be-latency-s2.data > data/1-4-mcast-be-s2.gpd
$REDUCECMD $TESTBASE/run/1-4/mcast-be-latency-s3.data > data/1-4-mcast-be-s3.gpd
$REDUCECMD $TESTBASE/run/1-4/mcast-be-latency-s4.data > data/1-4-mcast-be-s4.gpd
$REDUCECMD $TESTBASE/run/1-8/mcast-be-latency-s1.data > data/1-8-mcast-be-s1.gpd
$REDUCECMD $TESTBASE/run/1-8/mcast-be-latency-s2.data > data/1-8-mcast-be-s2.gpd
$REDUCECMD $TESTBASE/run/1-8/mcast-be-latency-s3.data > data/1-8-mcast-be-s3.gpd
$REDUCECMD $TESTBASE/run/1-8/mcast-be-latency-s4.data > data/1-8-mcast-be-s4.gpd
$REDUCECMD $TESTBASE/run/1-8/mcast-be-latency-s5.data > data/1-8-mcast-be-s5.gpd
$REDUCECMD $TESTBASE/run/1-8/mcast-be-latency-s6.data > data/1-8-mcast-be-s6.gpd
$REDUCECMD $TESTBASE/run/1-8/mcast-be-latency-s7.data > data/1-8-mcast-be-s7.gpd
$REDUCECMD $TESTBASE/run/1-8/mcast-be-latency-s8.data > data/1-8-mcast-be-s8.gpd
$REDUCECMD $TESTBASE/run/1-16/mcast-be-latency-s9.data > data/1-16-mcast-be-s9.gpd
$REDUCECMD $TESTBASE/run/1-16/mcast-be-latency-s10.data > data/1-16-mcast-be-s10.gpd
$REDUCECMD $TESTBASE/run/1-16/mcast-be-latency-s11.data > data/1-16-mcast-be-s11.gpd
$REDUCECMD $TESTBASE/run/1-16/mcast-be-latency-s12.data > data/1-16-mcast-be-s12.gpd
$REDUCECMD $TESTBASE/run/1-16/mcast-be-latency-s13.data > data/1-16-mcast-be-s13.gpd
$REDUCECMD $TESTBASE/run/1-16/mcast-be-latency-s14.data > data/1-16-mcast-be-s14.gpd
$REDUCECMD $TESTBASE/run/1-16/mcast-be-latency-s15.data > data/1-16-mcast-be-s15.gpd
$REDUCECMD $TESTBASE/run/1-16/mcast-be-latency-s16.data > data/1-16-mcast-be-s16.gpd

$REDUCECMD $TESTBASE/run/1-2/rtps-latency-s1.data > data/1-2-rtps-rel-s1.gpd
$REDUCECMD $TESTBASE/run/1-2/rtps-latency-s2.data > data/1-2-rtps-rel-s2.gpd
$REDUCECMD $TESTBASE/run/1-4/rtps-latency-s1.data > data/1-4-rtps-rel-s1.gpd
$REDUCECMD $TESTBASE/run/1-4/rtps-latency-s2.data > data/1-4-rtps-rel-s2.gpd
$REDUCECMD $TESTBASE/run/1-4/rtps-latency-s3.data > data/1-4-rtps-rel-s3.gpd
$REDUCECMD $TESTBASE/run/1-4/rtps-latency-s4.data > data/1-4-rtps-rel-s4.gpd
$REDUCECMD $TESTBASE/run/1-8/rtps-latency-s1.data > data/1-8-rtps-rel-s1.gpd
$REDUCECMD $TESTBASE/run/1-8/rtps-latency-s2.data > data/1-8-rtps-rel-s2.gpd
$REDUCECMD $TESTBASE/run/1-8/rtps-latency-s3.data > data/1-8-rtps-rel-s3.gpd
$REDUCECMD $TESTBASE/run/1-8/rtps-latency-s4.data > data/1-8-rtps-rel-s4.gpd
$REDUCECMD $TESTBASE/run/1-8/rtps-latency-s5.data > data/1-8-rtps-rel-s5.gpd
$REDUCECMD $TESTBASE/run/1-8/rtps-latency-s6.data > data/1-8-rtps-rel-s6.gpd
$REDUCECMD $TESTBASE/run/1-8/rtps-latency-s7.data > data/1-8-rtps-rel-s7.gpd
$REDUCECMD $TESTBASE/run/1-8/rtps-latency-s8.data > data/1-8-rtps-rel-s8.gpd
$REDUCECMD $TESTBASE/run/1-16/rtps-latency-s1.data > data/1-16-rtps-rel-s1.gpd
$REDUCECMD $TESTBASE/run/1-16/rtps-latency-s2.data > data/1-16-rtps-rel-s2.gpd
$REDUCECMD $TESTBASE/run/1-16/rtps-latency-s3.data > data/1-16-rtps-rel-s3.gpd
$REDUCECMD $TESTBASE/run/1-16/rtps-latency-s4.data > data/1-16-rtps-rel-s4.gpd
$REDUCECMD $TESTBASE/run/1-16/rtps-latency-s5.data > data/1-16-rtps-rel-s5.gpd
$REDUCECMD $TESTBASE/run/1-16/rtps-latency-s6.data > data/1-16-rtps-rel-s6.gpd
$REDUCECMD $TESTBASE/run/1-16/rtps-latency-s7.data > data/1-16-rtps-rel-s7.gpd
$REDUCECMD $TESTBASE/run/1-16/rtps-latency-s8.data > data/1-16-rtps-rel-s8.gpd
$REDUCECMD $TESTBASE/run/1-16/rtps-latency-s9.data > data/1-16-rtps-rel-s9.gpd
$REDUCECMD $TESTBASE/run/1-16/rtps-latency-s10.data > data/1-16-rtps-rel-s10.gpd
$REDUCECMD $TESTBASE/run/1-16/rtps-latency-s11.data > data/1-16-rtps-rel-s11.gpd
$REDUCECMD $TESTBASE/run/1-16/rtps-latency-s12.data > data/1-16-rtps-rel-s12.gpd
$REDUCECMD $TESTBASE/run/1-16/rtps-latency-s13.data > data/1-16-rtps-rel-s13.gpd
$REDUCECMD $TESTBASE/run/1-16/rtps-latency-s14.data > data/1-16-rtps-rel-s14.gpd
$REDUCECMD $TESTBASE/run/1-16/rtps-latency-s15.data > data/1-16-rtps-rel-s15.gpd
$REDUCECMD $TESTBASE/run/1-16/rtps-latency-s16.data > data/1-16-rtps-rel-s16.gpd
