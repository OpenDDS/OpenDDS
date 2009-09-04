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

# sed(1) scripts to apply to target files
SCRIPTS=`find $TOOLS_HOME/style/style.d -type f \
        -name '*.sed'`

for FILE in $FILES; do
  echo "Processing $FILE"

  # Apply astyle(1):
  astyle --quiet --options=$TOOLS_HOME/style/astyle.options $FILE

  # Apply individual sed(1) scripts:
  for SCRIPT in $SCRIPTS; do
    cp $FILE $FILE.orig
    sed -f $SCRIPT $FILE.orig > $FILE
  done
done
