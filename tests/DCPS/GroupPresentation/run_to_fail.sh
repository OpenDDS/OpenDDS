#! /bin/bash
export res="test PASSED."
while [ "$res" == "test PASSED." ]; do
  res=`./run_test.pl topic >test.out 2>&1; tail -1 test.out`
  echo $res

  grep DEADLOCK test.out
  if [ $? == 0 ]; then
    break;
  fi
done

awk -F@ '{print $3}' < test.out | awk -F: '{print $3}' | sort -u | grep "^[0-9][0-9]*$" > threads
for a in `cat threads`; do grep -n ":$a:" test.out > t$a.out; done
