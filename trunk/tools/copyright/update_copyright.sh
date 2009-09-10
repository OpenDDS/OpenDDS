#!/bin/sh
#
# $Id$
#

if [ ! $DDS_ROOT ]; then
  echo "ERROR: DDS_ROOT environment variable not set!"
  exit 1
fi 

TOOLS_HOME=$DDS_ROOT/tools

if [ $# = 0 ]; then
  echo "Usage: $0 file..."
  exit 1
fi

DATE=`date +%Y`
for FILE in $FILES; do
  echo "Processing $FILE"

  if [ ! -f "$FILE" ]; then
    echo "$FILE: No such file"
    exit 1
  fi

  cp $FILE $FILE.orig
  sed -e "s/\(Copyright\).*\(Object Computing, Inc.\)\$/\1 $DATE \2/g" \
         $FILE.orig > $FILE
done
