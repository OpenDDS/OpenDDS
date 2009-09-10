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

# sed(1) scripts to apply to target files
SCRIPTS=`find $TOOLS_HOME/style/style.d -type f \
        -name '*.sed'`

for FILE in "$@"; do
  echo "Processing $FILE"

  if [ ! -f "$FILE" ]; then
    echo "$FILE: No such file"
    exit 1
  fi

  # Apply astyle(1):
  astyle --quiet --options=$TOOLS_HOME/style/astyle.options $FILE

  # Apply individual sed(1) scripts:
  for SCRIPT in $SCRIPTS; do
    cp $FILE $FILE.orig
    sed -f $SCRIPT $FILE.orig > $FILE
  done
done
