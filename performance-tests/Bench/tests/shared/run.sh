#!/bin/bash -vx

export BENCHBASE=$DDS_ROOT/performance-tests/Bench
export TESTBASE=$BENCHBASE/tests/shared
export TESTCMD="$BENCHBASE/bin/run_test -t 60 -S -h localhost:2809 -P"

export TRANSPORT_SHMEM=$TESTBASE/transport-shmem.ini
export TRANSPORT_TCP=$TESTBASE/transport-tcp.ini
export TRANSPORT_UDP=$TESTBASE/transport-udp.ini
export TRANSPORT_MCAST_BE=$TESTBASE/transport-mcast-be.ini
export TRANSPORT_MCAST_REL=$TESTBASE/transport-mcast-rel.ini
export TRANSPORT_RTPS=$TESTBASE/transport-rtps.ini

mkdir -p run/1-1
pushd run/1-1
$TESTCMD -i $TRANSPORT_SHMEM -s $TESTBASE/s1.ini,$TESTBASE/p1.ini
mv latency-s1.data shmem-latency-s1.data
$TESTCMD -i $TRANSPORT_TCP -s $TESTBASE/s1.ini,$TESTBASE/p1.ini
mv latency-s1.data tcp-latency-s1.data
$TESTCMD -i $TRANSPORT_MCAST_REL -s $TESTBASE/s1.ini,$TESTBASE/p1.ini
mv latency-s1.data mcast-rel-latency-s1.data
$TESTCMD -i $TRANSPORT_UDP -s $TESTBASE/s1be.ini,$TESTBASE/p1be.ini
mv latency-s1.data udp-latency-s1.data
$TESTCMD -i $TRANSPORT_MCAST_BE -s $TESTBASE/s1be.ini,$TESTBASE/p1be.ini
mv latency-s1.data mcast-be-latency-s1.data
$TESTCMD -i $TRANSPORT_RTPS -s $TESTBASE/s1.ini,$TESTBASE/p1.ini
mv latency-s1.data rtps-latency-s1.data
popd

mkdir -p run/2-1
pushd run/2-1
$TESTCMD -i $TRANSPORT_SHMEM -s $TESTBASE/s1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini
mv latency-s1.data shmem-latency-s1.data
$TESTCMD -i $TRANSPORT_TCP -s $TESTBASE/s1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini
mv latency-s1.data tcp-latency-s1.data
$TESTCMD -i $TRANSPORT_MCAST_REL -s $TESTBASE/s1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini
mv latency-s1.data mcast-rel-latency-s1.data
$TESTCMD -i $TRANSPORT_UDP -s $TESTBASE/s1be.ini,$TESTBASE/p1be.ini,$TESTBASE/p1be.ini
mv latency-s1.data udp-latency-s1.data
$TESTCMD -i $TRANSPORT_MCAST_BE -s $TESTBASE/s1be.ini,$TESTBASE/p1be.ini,$TESTBASE/p1be.ini
mv latency-s1.data mcast-be-latency-s1.data
$TESTCMD -i $TRANSPORT_RTPS -s $TESTBASE/s1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini
mv latency-s1.data rtps-latency-s1.data
popd

mkdir -p run/4-1
pushd run/4-1
$TESTCMD -i $TRANSPORT_SHMEM -s $TESTBASE/s1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini
mv latency-s1.data shmem-latency-s1.data
$TESTCMD -i $TRANSPORT_TCP -s $TESTBASE/s1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini
mv latency-s1.data tcp-latency-s1.data
$TESTCMD -i $TRANSPORT_MCAST_REL -s $TESTBASE/s1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini
mv latency-s1.data mcast-rel-latency-s1.data
$TESTCMD -i $TRANSPORT_UDP -s $TESTBASE/s1be.ini,$TESTBASE/p1be.ini,$TESTBASE/p1be.ini,$TESTBASE/p1be.ini,$TESTBASE/p1be.ini
mv latency-s1.data udp-latency-s1.data
$TESTCMD -i $TRANSPORT_MCAST_BE -s $TESTBASE/s1be.ini,$TESTBASE/p1be.ini,$TESTBASE/p1be.ini,$TESTBASE/p1be.ini,$TESTBASE/p1be.ini
mv latency-s1.data mcast-be-latency-s1.data
$TESTCMD -i $TRANSPORT_RTPS -s $TESTBASE/s1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini
mv latency-s1.data rtps-latency-s1.data
popd

mkdir -p run/8-1
pushd run/8-1
$TESTCMD -i $TRANSPORT_SHMEM -s $TESTBASE/s1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini
mv latency-s1.data shmem-latency-s1.data
$TESTCMD -i $TRANSPORT_TCP -s $TESTBASE/s1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini
mv latency-s1.data tcp-latency-s1.data
$TESTCMD -i $TRANSPORT_MCAST_REL -s $TESTBASE/s1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini
mv latency-s1.data mcast-rel-latency-s1.data
$TESTCMD -i $TRANSPORT_UDP -s $TESTBASE/s1be.ini,$TESTBASE/p1be.ini,$TESTBASE/p1be.ini,$TESTBASE/p1be.ini,$TESTBASE/p1be.ini,$TESTBASE/p1be.ini,$TESTBASE/p1be.ini,$TESTBASE/p1be.ini,$TESTBASE/p1be.ini
mv latency-s1.data udp-latency-s1.data
$TESTCMD -i $TRANSPORT_MCAST_BE -s $TESTBASE/s1be.ini,$TESTBASE/p1be.ini,$TESTBASE/p1be.ini,$TESTBASE/p1be.ini,$TESTBASE/p1be.ini,$TESTBASE/p1be.ini,$TESTBASE/p1be.ini,$TESTBASE/p1be.ini,$TESTBASE/p1be.ini
mv latency-s1.data mcast-be-latency-s1.data
$TESTCMD -i $TRANSPORT_RTPS -s $TESTBASE/s1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini
mv latency-s1.data rtps-latency-s1.data
popd

mkdir -p run/16-1
pushd run/16-1
$TESTCMD -i $TRANSPORT_SHMEM -s $TESTBASE/s1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini
mv latency-s1.data shmem-latency-s1.data
$TESTCMD -i $TRANSPORT_TCP -s $TESTBASE/s1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini
mv latency-s1.data tcp-latency-s1.data
$TESTCMD -i $TRANSPORT_MCAST_REL -s $TESTBASE/s1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini
mv latency-s1.data mcast-rel-latency-s1.data
$TESTCMD -i $TRANSPORT_UDP -s $TESTBASE/s1be.ini,$TESTBASE/p1be.ini,$TESTBASE/p1be.ini,$TESTBASE/p1be.ini,$TESTBASE/p1be.ini,$TESTBASE/p1be.ini,$TESTBASE/p1be.ini,$TESTBASE/p1be.ini,$TESTBASE/p1be.ini,$TESTBASE/p1be.ini,$TESTBASE/p1be.ini,$TESTBASE/p1be.ini,$TESTBASE/p1be.ini,$TESTBASE/p1be.ini,$TESTBASE/p1be.ini,$TESTBASE/p1be.ini,$TESTBASE/p1be.ini
mv latency-s1.data udp-latency-s1.data
$TESTCMD -i $TRANSPORT_MCAST_BE -s $TESTBASE/s1be.ini,$TESTBASE/p1be.ini,$TESTBASE/p1be.ini,$TESTBASE/p1be.ini,$TESTBASE/p1be.ini,$TESTBASE/p1be.ini,$TESTBASE/p1be.ini,$TESTBASE/p1be.ini,$TESTBASE/p1be.ini,$TESTBASE/p1be.ini,$TESTBASE/p1be.ini,$TESTBASE/p1be.ini,$TESTBASE/p1be.ini,$TESTBASE/p1be.ini,$TESTBASE/p1be.ini,$TESTBASE/p1be.ini,$TESTBASE/p1be.ini
mv latency-s1.data mcast-be-latency-s1.data
$TESTCMD -i $TRANSPORT_RTPS -s $TESTBASE/s1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini,$TESTBASE/p1.ini
mv latency-s1.data rtps-latency-s1.data
popd

mkdir -p run/1-2
pushd run/1-2
$TESTCMD -i $TRANSPORT_SHMEM -s $TESTBASE/s1.ini,$TESTBASE/s2.ini,$TESTBASE/p1-2.ini
mv latency-s1.data shmem-latency-s1.data
mv latency-s2.data shmem-latency-s2.data
$TESTCMD -i $TRANSPORT_TCP -s $TESTBASE/s1.ini,$TESTBASE/s2.ini,$TESTBASE/p1-2.ini
mv latency-s1.data tcp-latency-s1.data
mv latency-s2.data tcp-latency-s2.data
$TESTCMD -i $TRANSPORT_MCAST_REL -s $TESTBASE/s1.ini,$TESTBASE/s2.ini,$TESTBASE/p1-2.ini
mv latency-s1.data mcast-rel-latency-s1.data
mv latency-s2.data mcast-rel-latency-s2.data
$TESTCMD -i $TRANSPORT_UDP -s $TESTBASE/s1be.ini,$TESTBASE/s2be.ini,$TESTBASE/p1-2be.ini
mv latency-s1.data udp-latency-s1.data
mv latency-s2.data udp-latency-s2.data
$TESTCMD -i $TRANSPORT_MCAST_BE -s $TESTBASE/s1be.ini,$TESTBASE/s2be.ini,$TESTBASE/p1-2be.ini
mv latency-s1.data mcast-be-latency-s1.data
mv latency-s2.data mcast-be-latency-s2.data
$TESTCMD -i $TRANSPORT_RTPS -s $TESTBASE/s1.ini,$TESTBASE/s2.ini,$TESTBASE/p1-2.ini
mv latency-s1.data rtps-latency-s1.data
mv latency-s2.data rtps-latency-s2.data
popd

mkdir -p run/1-4
pushd run/1-4
$TESTCMD -i $TRANSPORT_SHMEM -s $TESTBASE/s1.ini,$TESTBASE/s2.ini,$TESTBASE/s3.ini,$TESTBASE/s4.ini,$TESTBASE/p1-4.ini
mv latency-s1.data shmem-latency-s1.data
mv latency-s2.data shmem-latency-s2.data
mv latency-s3.data shmem-latency-s3.data
mv latency-s4.data shmem-latency-s4.data
$TESTCMD -i $TRANSPORT_TCP -s $TESTBASE/s1.ini,$TESTBASE/s2.ini,$TESTBASE/s3.ini,$TESTBASE/s4.ini,$TESTBASE/p1-4.ini
mv latency-s1.data tcp-latency-s1.data
mv latency-s2.data tcp-latency-s2.data
mv latency-s3.data tcp-latency-s3.data
mv latency-s4.data tcp-latency-s4.data
$TESTCMD -i $TRANSPORT_MCAST_REL -s $TESTBASE/s1.ini,$TESTBASE/s2.ini,$TESTBASE/s3.ini,$TESTBASE/s4.ini,$TESTBASE/p1-4.ini
mv latency-s1.data mcast-rel-latency-s1.data
mv latency-s2.data mcast-rel-latency-s2.data
mv latency-s3.data mcast-rel-latency-s3.data
mv latency-s4.data mcast-rel-latency-s4.data
$TESTCMD -i $TRANSPORT_UDP -s $TESTBASE/s1be.ini,$TESTBASE/s2be.ini,$TESTBASE/s3be.ini,$TESTBASE/s4be.ini,$TESTBASE/p1-4be.ini
mv latency-s1.data udp-latency-s1.data
mv latency-s2.data udp-latency-s2.data
mv latency-s3.data udp-latency-s3.data
mv latency-s4.data udp-latency-s4.data
$TESTCMD -i $TRANSPORT_MCAST_BE -s $TESTBASE/s1be.ini,$TESTBASE/s2be.ini,$TESTBASE/s3be.ini,$TESTBASE/s4be.ini,$TESTBASE/p1-4be.ini
mv latency-s1.data mcast-be-latency-s1.data
mv latency-s2.data mcast-be-latency-s2.data
mv latency-s3.data mcast-be-latency-s3.data
mv latency-s4.data mcast-be-latency-s4.data
$TESTCMD -i $TRANSPORT_RTPS -s $TESTBASE/s1.ini,$TESTBASE/s2.ini,$TESTBASE/s3.ini,$TESTBASE/s4.ini,$TESTBASE/p1-4.ini
mv latency-s1.data rtps-latency-s1.data
mv latency-s2.data rtps-latency-s2.data
mv latency-s3.data rtps-latency-s3.data
mv latency-s4.data rtps-latency-s4.data
popd

mkdir -p run/1-8
pushd run/1-8
$TESTCMD -i $TRANSPORT_SHMEM -s $TESTBASE/s1.ini,$TESTBASE/s2.ini,$TESTBASE/s3.ini,$TESTBASE/s4.ini,$TESTBASE/s5.ini,$TESTBASE/s6.ini,$TESTBASE/s7.ini,$TESTBASE/s8.ini,$TESTBASE/p1-8.ini
mv latency-s1.data shmem-latency-s1.data
mv latency-s2.data shmem-latency-s2.data
mv latency-s3.data shmem-latency-s3.data
mv latency-s4.data shmem-latency-s4.data
mv latency-s5.data shmem-latency-s5.data
mv latency-s6.data shmem-latency-s6.data
mv latency-s7.data shmem-latency-s7.data
mv latency-s8.data shmem-latency-s8.data
$TESTCMD -i $TRANSPORT_TCP -s $TESTBASE/s1.ini,$TESTBASE/s2.ini,$TESTBASE/s3.ini,$TESTBASE/s4.ini,$TESTBASE/s5.ini,$TESTBASE/s6.ini,$TESTBASE/s7.ini,$TESTBASE/s8.ini,$TESTBASE/p1-8.ini
mv latency-s1.data tcp-latency-s1.data
mv latency-s2.data tcp-latency-s2.data
mv latency-s3.data tcp-latency-s3.data
mv latency-s4.data tcp-latency-s4.data
mv latency-s5.data tcp-latency-s5.data
mv latency-s6.data tcp-latency-s6.data
mv latency-s7.data tcp-latency-s7.data
mv latency-s8.data tcp-latency-s8.data
$TESTCMD -i $TRANSPORT_MCAST_REL -s $TESTBASE/s1.ini,$TESTBASE/s2.ini,$TESTBASE/s3.ini,$TESTBASE/s4.ini,$TESTBASE/s5.ini,$TESTBASE/s6.ini,$TESTBASE/s7.ini,$TESTBASE/s8.ini,$TESTBASE/p1-8.ini
mv latency-s1.data mcast-rel-latency-s1.data
mv latency-s2.data mcast-rel-latency-s2.data
mv latency-s3.data mcast-rel-latency-s3.data
mv latency-s4.data mcast-rel-latency-s4.data
mv latency-s5.data mcast-rel-latency-s5.data
mv latency-s6.data mcast-rel-latency-s6.data
mv latency-s7.data mcast-rel-latency-s7.data
mv latency-s8.data mcast-rel-latency-s8.data
$TESTCMD -i $TRANSPORT_UDP -s $TESTBASE/s1be.ini,$TESTBASE/s2be.ini,$TESTBASE/s3be.ini,$TESTBASE/s4be.ini,$TESTBASE/s5be.ini,$TESTBASE/s6be.ini,$TESTBASE/s7be.ini,$TESTBASE/s8be.ini,$TESTBASE/p1-8be.ini
mv latency-s1.data udp-latency-s1.data
mv latency-s2.data udp-latency-s2.data
mv latency-s3.data udp-latency-s3.data
mv latency-s4.data udp-latency-s4.data
mv latency-s5.data udp-latency-s5.data
mv latency-s6.data udp-latency-s6.data
mv latency-s7.data udp-latency-s7.data
mv latency-s8.data udp-latency-s8.data
$TESTCMD -i $TRANSPORT_MCAST_BE -s $TESTBASE/s1be.ini,$TESTBASE/s2be.ini,$TESTBASE/s3be.ini,$TESTBASE/s4be.ini,$TESTBASE/s5be.ini,$TESTBASE/s6be.ini,$TESTBASE/s7be.ini,$TESTBASE/s8be.ini,$TESTBASE/p1-8be.ini
mv latency-s1.data mcast-be-latency-s1.data
mv latency-s2.data mcast-be-latency-s2.data
mv latency-s3.data mcast-be-latency-s3.data
mv latency-s4.data mcast-be-latency-s4.data
mv latency-s5.data mcast-be-latency-s5.data
mv latency-s6.data mcast-be-latency-s6.data
mv latency-s7.data mcast-be-latency-s7.data
mv latency-s8.data mcast-be-latency-s8.data
$TESTCMD -i $TRANSPORT_RTPS -s $TESTBASE/s1.ini,$TESTBASE/s2.ini,$TESTBASE/s3.ini,$TESTBASE/s4.ini,$TESTBASE/s5.ini,$TESTBASE/s6.ini,$TESTBASE/s7.ini,$TESTBASE/s8.ini,$TESTBASE/p1-8.ini
mv latency-s1.data rtps-latency-s1.data
mv latency-s2.data rtps-latency-s2.data
mv latency-s3.data rtps-latency-s3.data
mv latency-s4.data rtps-latency-s4.data
mv latency-s5.data rtps-latency-s5.data
mv latency-s6.data rtps-latency-s6.data
mv latency-s7.data rtps-latency-s7.data
mv latency-s8.data rtps-latency-s8.data
popd

mkdir -p run/1-16
pushd run/1-16
$TESTCMD -i $TRANSPORT_SHMEM -s $TESTBASE/s1.ini,$TESTBASE/s2.ini,$TESTBASE/s3.ini,$TESTBASE/s4.ini,$TESTBASE/s5.ini,$TESTBASE/s6.ini,$TESTBASE/s7.ini,$TESTBASE/s8.ini,$TESTBASE/s9.ini,$TESTBASE/s10.ini,$TESTBASE/s11.ini,$TESTBASE/s12.ini,$TESTBASE/s13.ini,$TESTBASE/s14.ini,$TESTBASE/s15.ini,$TESTBASE/s16.ini,$TESTBASE/p1-16.ini
mv latency-s1.data shmem-latency-s1.data
mv latency-s2.data shmem-latency-s2.data
mv latency-s3.data shmem-latency-s3.data
mv latency-s4.data shmem-latency-s4.data
mv latency-s5.data shmem-latency-s5.data
mv latency-s6.data shmem-latency-s6.data
mv latency-s7.data shmem-latency-s7.data
mv latency-s8.data shmem-latency-s8.data
mv latency-s9.data shmem-latency-s9.data
mv latency-s10.data shmem-latency-s10.data
mv latency-s11.data shmem-latency-s11.data
mv latency-s12.data shmem-latency-s12.data
mv latency-s13.data shmem-latency-s13.data
mv latency-s14.data shmem-latency-s14.data
mv latency-s15.data shmem-latency-s15.data
mv latency-s16.data shmem-latency-s16.data
$TESTCMD -i $TRANSPORT_TCP -s $TESTBASE/s1.ini,$TESTBASE/s2.ini,$TESTBASE/s3.ini,$TESTBASE/s4.ini,$TESTBASE/s5.ini,$TESTBASE/s6.ini,$TESTBASE/s7.ini,$TESTBASE/s16.ini,$TESTBASE/s9.ini,$TESTBASE/s10.ini,$TESTBASE/s11.ini,$TESTBASE/s12.ini,$TESTBASE/s13.ini,$TESTBASE/s14.ini,$TESTBASE/s15.ini,$TESTBASE/s16.ini,$TESTBASE/p1-16.ini
mv latency-s1.data tcp-latency-s1.data
mv latency-s2.data tcp-latency-s2.data
mv latency-s3.data tcp-latency-s3.data
mv latency-s4.data tcp-latency-s4.data
mv latency-s5.data tcp-latency-s5.data
mv latency-s6.data tcp-latency-s6.data
mv latency-s7.data tcp-latency-s7.data
mv latency-s8.data tcp-latency-s8.data
mv latency-s9.data tcp-latency-s9.data
mv latency-s10.data tcp-latency-s10.data
mv latency-s11.data tcp-latency-s11.data
mv latency-s12.data tcp-latency-s12.data
mv latency-s13.data tcp-latency-s13.data
mv latency-s14.data tcp-latency-s14.data
mv latency-s15.data tcp-latency-s15.data
mv latency-s16.data tcp-latency-s16.data
$TESTCMD -i $TRANSPORT_MCAST_REL -s $TESTBASE/s1.ini,$TESTBASE/s2.ini,$TESTBASE/s3.ini,$TESTBASE/s4.ini,$TESTBASE/s5.ini,$TESTBASE/s6.ini,$TESTBASE/s7.ini,$TESTBASE/s16.ini,$TESTBASE/s9.ini,$TESTBASE/s10.ini,$TESTBASE/s11.ini,$TESTBASE/s12.ini,$TESTBASE/s13.ini,$TESTBASE/s14.ini,$TESTBASE/s15.ini,$TESTBASE/s16.ini,$TESTBASE/p1-16.ini
mv latency-s1.data mcast-rel-latency-s1.data
mv latency-s2.data mcast-rel-latency-s2.data
mv latency-s3.data mcast-rel-latency-s3.data
mv latency-s4.data mcast-rel-latency-s4.data
mv latency-s5.data mcast-rel-latency-s5.data
mv latency-s6.data mcast-rel-latency-s6.data
mv latency-s7.data mcast-rel-latency-s7.data
mv latency-s8.data mcast-rel-latency-s8.data
mv latency-s9.data mcast-rel-latency-s9.data
mv latency-s10.data mcast-rel-latency-s10.data
mv latency-s11.data mcast-rel-latency-s11.data
mv latency-s12.data mcast-rel-latency-s12.data
mv latency-s13.data mcast-rel-latency-s13.data
mv latency-s14.data mcast-rel-latency-s14.data
mv latency-s15.data mcast-rel-latency-s15.data
mv latency-s16.data mcast-rel-latency-s16.data
$TESTCMD -i $TRANSPORT_UDP -s $TESTBASE/s1be.ini,$TESTBASE/s2be.ini,$TESTBASE/s3be.ini,$TESTBASE/s4be.ini,$TESTBASE/s5be.ini,$TESTBASE/s6be.ini,$TESTBASE/s7be.ini,$TESTBASE/s16be.ini,$TESTBASE/s9be.ini,$TESTBASE/s10be.ini,$TESTBASE/s11be.ini,$TESTBASE/s12be.ini,$TESTBASE/s13be.ini,$TESTBASE/s14be.ini,$TESTBASE/s15be.ini,$TESTBASE/s16be.ini,$TESTBASE/p1-16be.ini
mv latency-s1.data udp-latency-s1.data
mv latency-s2.data udp-latency-s2.data
mv latency-s3.data udp-latency-s3.data
mv latency-s4.data udp-latency-s4.data
mv latency-s5.data udp-latency-s5.data
mv latency-s6.data udp-latency-s6.data
mv latency-s7.data udp-latency-s7.data
mv latency-s8.data udp-latency-s8.data
mv latency-s9.data udp-latency-s9.data
mv latency-s10.data udp-latency-s10.data
mv latency-s11.data udp-latency-s11.data
mv latency-s12.data udp-latency-s12.data
mv latency-s13.data udp-latency-s13.data
mv latency-s14.data udp-latency-s14.data
mv latency-s15.data udp-latency-s15.data
mv latency-s16.data udp-latency-s16.data
$TESTCMD -i $TRANSPORT_MCAST_BE -s $TESTBASE/s1be.ini,$TESTBASE/s2be.ini,$TESTBASE/s3be.ini,$TESTBASE/s4be.ini,$TESTBASE/s5be.ini,$TESTBASE/s6be.ini,$TESTBASE/s7be.ini,$TESTBASE/s16be.ini,$TESTBASE/s9be.ini,$TESTBASE/s10be.ini,$TESTBASE/s11be.ini,$TESTBASE/s12be.ini,$TESTBASE/s13be.ini,$TESTBASE/s14be.ini,$TESTBASE/s15be.ini,$TESTBASE/s16be.ini,$TESTBASE/p1-16be.ini
mv latency-s1.data mcast-be-latency-s1.data
mv latency-s2.data mcast-be-latency-s2.data
mv latency-s3.data mcast-be-latency-s3.data
mv latency-s4.data mcast-be-latency-s4.data
mv latency-s5.data mcast-be-latency-s5.data
mv latency-s6.data mcast-be-latency-s6.data
mv latency-s7.data mcast-be-latency-s7.data
mv latency-s8.data mcast-be-latency-s8.data
mv latency-s9.data mcast-be-latency-s9.data
mv latency-s10.data mcast-be-latency-s10.data
mv latency-s11.data mcast-be-latency-s11.data
mv latency-s12.data mcast-be-latency-s12.data
mv latency-s13.data mcast-be-latency-s13.data
mv latency-s14.data mcast-be-latency-s14.data
mv latency-s15.data mcast-be-latency-s15.data
mv latency-s16.data mcast-be-latency-s16.data
$TESTCMD -i $TRANSPORT_RTPS -s $TESTBASE/s1.ini,$TESTBASE/s2.ini,$TESTBASE/s3.ini,$TESTBASE/s4.ini,$TESTBASE/s5.ini,$TESTBASE/s6.ini,$TESTBASE/s7.ini,$TESTBASE/s16.ini,$TESTBASE/s9.ini,$TESTBASE/s10.ini,$TESTBASE/s11.ini,$TESTBASE/s12.ini,$TESTBASE/s13.ini,$TESTBASE/s14.ini,$TESTBASE/s15.ini,$TESTBASE/s16.ini,$TESTBASE/p1-16.ini
mv latency-s1.data rtps-latency-s1.data
mv latency-s2.data rtps-latency-s2.data
mv latency-s3.data rtps-latency-s3.data
mv latency-s4.data rtps-latency-s4.data
mv latency-s5.data rtps-latency-s5.data
mv latency-s6.data rtps-latency-s6.data
mv latency-s7.data rtps-latency-s7.data
mv latency-s8.data rtps-latency-s8.data
mv latency-s9.data rtps-latency-s9.data
mv latency-s10.data rtps-latency-s10.data
mv latency-s11.data rtps-latency-s11.data
mv latency-s12.data rtps-latency-s12.data
mv latency-s13.data rtps-latency-s13.data
mv latency-s14.data rtps-latency-s14.data
mv latency-s15.data rtps-latency-s15.data
mv latency-s16.data rtps-latency-s16.data
popd

