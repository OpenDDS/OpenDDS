#!/bin/sh
#
# $Id$
#

if [ $# -lt 1 ]; then
  echo "usage: $0 test [argument...]"
  exit 1
fi

PASS=1
while [ $? -eq 0 ]; do
  echo "TEST PASS: $PASS" && $@
  PASS=`expr $PASS + 1`
done
