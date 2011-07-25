#!/bin/sh

progdir=`dirname "$0"`
java -jar -Dapp.name=__APPNAME__ \
     -Dopendds.xsd.file=$progdir/xsd/OpenDDSXMI.xsd \
     -Dcodegen.xsd.file=$progdir/xsd/GeneratorXMI.xsd \
     $progdir/lib/__APPNAME__.jar $*
