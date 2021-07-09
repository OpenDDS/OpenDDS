eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

use Env (DDS_ROOT);
use lib "$DDS_ROOT/bin";
use File::Compare;
use strict;

my $gencmd = "$ENV{DDS_ROOT}/bin/opendds_idl -Sa -St sample.idl";
system($gencmd) == 0 or die "ERROR: could not run $gencmd $?\n";
rename("sampleTypeSupportImpl.cpp","pass1");
system($gencmd) == 0 or die "ERROR: could not run $gencmd $?\n";
my $result = compare("pass1","sampleTypeSupportImpl.cpp");
unlink("pass1");
unlink("sampleT*");
if ($result == 0)
{
    print "test PASSED\n";
    exit 0;
}

print "test FAILED\n";
exit 1;

