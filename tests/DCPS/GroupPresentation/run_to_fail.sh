#! /bin/bash
export res="test PASSED."
while [ "$res" == "test PASSED." ]; do
  res=`./run_test.pl topic >test.out 2>&1; tail -1 test.out`
  echo $res
done

awk -F@ '{print $3}' < test.out | awk -F: '{print $3}' | sort -u | grep -v "^$" > threads
for a in `cat threads`; do grep -n ":$a:" test.out > t$a.out; done
