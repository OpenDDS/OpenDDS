eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

use Env (DDS_ROOT);
use lib "$DDS_ROOT/bin";

use strict;

my $gencmd = "$ENV{DDS_ROOT}/bin/opendds_idl -Sa -St sample.idl";
system($gencmd);
system("mv sampleTypeSupportImpl.cpp pass1");
system($gencmd);
my $result = system("diff pass1 sampleTypeSupportImpl.cpp");
system("rm pass1 sampleT*");
if ($result == "")
{
    print "test PASSED\n";
    exit 0;
}

print "test FAILED\n";
exit 1;

