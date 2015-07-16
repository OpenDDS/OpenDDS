eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
     & eval 'exec perl -S $0 $argv:q'
     if 0;

# -*- perl -*-

use Env(DDS_ROOT);
use Env(JAVA_HOME);

use strict;

my $dest = shift;
if ($dest eq "") {
    $dest = "$DDS_ROOT/java/docs/api";
}

my @args = (
    "$JAVA_HOME/bin/javadoc",
    "-quiet",
    "-overview", "$DDS_ROOT/java/docs/overview.html",
    "-protected",
    "-source", "1.5",
    "-classpath", "$DDS_ROOT/lib/i2jrt.jar",
    "-sourcepath", "$DDS_ROOT/java/dds",
    "-d", "$dest",
    "-version",
    "-author",
    "-windowtitle", "OpenDDS_DCPS",
    "-doctitle", "OpenDDS DCPS API Specification",
    "-link", "http://download.oracle.com/javase/1.5.0/docs/api/",
# Package list:
    "DDS",
    "OpenDDS",
    "OpenDDS.DCPS",
    "OpenDDS.DCPS.transport"
);
system(@args);
