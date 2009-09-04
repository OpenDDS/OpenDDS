#!/bin/sh
#
# $Id$
#

if [ ! $DDS_ROOT ]; then
  echo "ERROR: DDS_ROOT environment variable not set!"
  exit 1
fi 

TOOLS_HOME=$DDS_ROOT/tools

# Files to process; if no arguments are passed, then
# all source files will be processed.
if [ $# != 0 ]; then
  FILES="$@"
else
  echo "WARNING: Processing ALL files in $DDS_ROOT!"
  FILES=`find $DDS_ROOT -type f \
        -name '*.h' -o -name '*.cpp' -o -name '*.inl'`
fi

DATE=`date +%Y`
for FILE in $FILES; do
  echo "Processing $FILE"

  cp $FILE $FILE.orig
  sed -e "s/\(Copyright\).*\(Object Computing, Inc.\)\$/\1 $DATE \2/g" \
         $FILE.orig > $FILE
done
