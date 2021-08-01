#! /bin/bash
export count=0;
export res="test PASSED."
while [ "$res" == "test PASSED." ]; do
  count=`expr $count + 1`
  res=`./run_test.pl topic >test.out 2>&1; tail -1 test.out`
  if [ `expr $count % 10` == 0 ]; then
    echo passed $count executions
  fi
done

echo failed on execution $count
