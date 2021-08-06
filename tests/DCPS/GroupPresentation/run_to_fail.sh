#! /bin/bash
rm -f t*out
export count=0;
export res="test PASSED."
while [ "$res" == "test PASSED." ]; do
  count=`expr $count + 1`
  res=`./run_test.pl topic >test.out 2>&1; tail -1 test.out`
  grep -q "map size = 2" test.out
  if [ $? == 0 ]; then
    echo  instance map size of 2 on run $count
    cp test.out im$count.out
  fi
  grep -q depth test.out
  if [ $? == 0 ]; then
    echo lock recursion encounterd in run $count
    cp test.out lr$count.out
  fi
  grep -q "failed to find" test.out
  if [ $? == 0 ]; then
    echo premature disconnect detected on run $count
    break;
  fi
  if [ `expr $count % 10` == 0 ]; then
    echo passed $count runs
  fi
  if [ -e wantpass ]; then
    rm wantpass
    break
  fi
done
if [ -e failcount ]; then
  export gpfail=`cat failcount`
fi
if [ -z "$gpfail" ]; then
  export gpfail=1
else
  export gpfail=`expr $gpfail + 1`
fi
echo failed on execution $count
awk -F@ '{print $3}' < test.out | awk -F: '{print $3}' | sort -u | grep "^[0-9][0-9]*$" > threads
for a in `cat threads`; do grep -n ":$a:" test.out > t$a.out; done
tar zcf f${gpfail}.tgz t*out
echo $gpfail > failcount
sudo cp f${gpfail}.tgz ~/projects/opes21
