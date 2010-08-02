eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# $Id$
# -*- perl -*-

# Locate the PLUGIN
use FindBin;
my $PLUGINROOT;
BEGIN { $PLUGINROOT = "$FindBin::Bin/.."; }

my $XJCCMD = "xjc";
my $XSDFILES = "$PLUGINROOT/xml/Generator.xsd "
             . "$PLUGINROOT/xml/externalfiles.xsd ";

chdir "$PLUGINROOT/src";
system("$XJCCMD -p org.opendds.modeling.sdk.codegen $XSDFILES");

