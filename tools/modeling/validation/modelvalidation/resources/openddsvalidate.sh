#!/bin/sh

progdir=`dirname "$0"`
java -jar -Dopendds.xsd.file=$progdir/xsd/OpenDDSXMI.xsd $progdir/__APPNAME__.jar $*