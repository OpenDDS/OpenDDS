#!/bin/sh
#
# $Id$
#

if [ ! "$DDS_ROOT" ]; then
  echo "ERROR: DDS_ROOT environment variable not set!"
  exit 1
fi

TOOLS_HOME="$DDS_ROOT/tools"

if [ $# = 0 ]; then
  echo "Usage: `basename $0` file..."
  exit 1
fi

for FILE in "$@"; do
  if [ ! -f "$FILE" ]; then
    echo "$FILE: No such file"
    exit 1
  fi

  echo "Processing $FILE"

  cp "$FILE" "$FILE.orig" # create working copy
  cat "$TOOLS_HOME/copyright/COPYRIGHT" "$FILE.orig" > "$FILE"
done
