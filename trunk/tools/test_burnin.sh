#!/bin/sh
#
# $Id$
#

if [ $# -lt 1 ]; then
  echo "usage: $0 test [argument...]"
  exit 1
fi

while [ $? -eq 0 ]; do
  PASS=`expr ${PASS:=0} + 1`
  echo "TEST PASS: $PASS" && $@
done
