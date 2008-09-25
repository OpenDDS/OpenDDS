eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
     & eval 'exec perl -S $0 $argv:q'
     if 0;

# $Id$
# -*- perl -*-

use Env(DDS_ROOT);
use Env(JAVA_HOME);

use strict;

my @args = (
    "$JAVA_HOME/bin/javadoc",
    "-overview", "$DDS_ROOT/java/docs/overview.html",
    "-protected",
    "-source", "1.5",
    "-sourcepath", "$DDS_ROOT/java/dds",
    "-d", "$DDS_ROOT/java/docs/api",
    "-version",
    "-author",
    "-windowtitle", "OpenDDS_DCPS",
    "-doctitle", "OpenDDS DCPS API Specification",
    "-link", "http://java.sun.com/j2se/1.5.0/docs/api/",
# Package list:
    "DDS",
    "OpenDDS",
    "OpenDDS.DCPS",
    "OpenDDS.DCPS.transport"
);
system(@args);
