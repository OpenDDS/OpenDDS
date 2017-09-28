#!/bin/bash
TESTBASE=$( cd "$( dirname ${BASH_SOURCE[0]} )" && pwd )
PROJECTBASE=$( cd $TESTBASE/../.. && pwd )

if [ -f $PROJECTBASE/bin/bin/DCPSInfoRepo ]; then
    DDS_ROOT=$PROJECTBASE
else
    DDS_ROOT=$PROJECTBASE/../..
fi

REPOHOST=`hostname`
REPOPORT=2809
TEST_DURATION=120


REMOTE_HOST=dds-perf-test2

CONFIGS="rtps udp tcp multi-be multi-rel"
SIZES="50 100 250 500 1000 2500 5000 8000 16000 32000"

$DDS_ROOT/bin/DCPSInfoRepo -ORBListenEndpoints iiop://$REPOHOST:$REPOPORT & export PREO_PID=$!

ssh $REMOTE_HOST "sed 's/RELIABLE/BEST_EFFORT/' $TESTBASE/p2.ini > $TESTBASE/p2-be.ini"

for sz in $SIZES; do
  sed "s/RELIABLE/BEST_EFFORT/" $TESTBASE/p1-$sz.ini > $TESTBASE/p1-$sz-be.ini
fi


for conf in $CONFIGS; do
  mkdir -p $conf
  cd $conf
  TRANSPORTCONFIG=$PROJECTBASE/etc/transport-$conf.ini

  if [[ "$conf" == "udp" || "$conf" == "multi-rel" ]]; then
      suffix="-be"
  fi

  for sz in $SIZES; do
    echo ============ Runing $conf $sz
    ssh $REMOTE_HOST "$PROJECTBASE/bin/run_test -P -t $TEST_DURATION -h $REPOHOST:$REPOPORT -i $TRANSPORTCONFIG -s $TESTBASE/p2$suffix.ini" &
    $PROJECTBASE/bin/run_test -P -t $TEST_DURATION -h $REPOHOST:$REPOPORT -i $TRANSPORTCONFIG -s $TESTBASE/p1-$sz$suffix.ini 2>&1 >  p1-$sz.log
  done
  cd ..
done

kill -9 $PREO_PID

SERVERPORT=3001

for conf in tcp udp; do
  mkdir -p raw-$conf
  cd raw-$conf
  for sz in $SIZES; do
    echo ============ Runing raw-$conf $sz
    ssh $REMOTE_HOST "$PROJECTBASE/bin/$conf_latency -m $sz -s $SERVERPORT" &
    $PROJECTBASE/bin/$conf_latency -d $TEST_DURATION -m $sz -c $REMOTE_HOST:$SERVERPORT 2>&1 >  p1-$sz.log
  done
  cd ..
done