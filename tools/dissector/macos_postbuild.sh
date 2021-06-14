#!/bin/bash

# This script takes care of two problems with the dissector file on macOS:
#
#   $DYLD_LIBRARY_PATH is not persevered between processes after
#   v10.11 "El Capitan" for security reasons and the rpath for the OpenDDS
#   libraries is relative. To remedy this we add a absolute rpath to
#   $DDS_ROOT/lib.
#
#   It produces dylib files which Wireshark will accept but only if they have
#   a "so" file extension. To remedy this we just make a copy with the "so"
#   extension.

filename="OpenDDS_Dissector"
d="$filename.dylib"
s="$filename.so"

# if there is a dylib file and either no so file or the dylib file is newer:
if [ -f "$d" -a \( \( ! -f "$s" \) -o \( "$d" -nt "$s" \) \) ]; then
  # then produce a so file:
  # (Since only macOS will produce dylib files other unixes will skip this)
  install_name_tool -add_rpath "$DDS_ROOT/lib" "$d"
  cp "$d" "$s"
fi
