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

for FILE in $FILES; do
  echo "Processing $FILE"

  cp $FILE $FILE.orig
  cat $TOOLS_HOME/copyright/COPYRIGHT $FILE.orig > $FILE
done
