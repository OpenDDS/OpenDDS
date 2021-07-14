eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

use Env qw(DDS_ROOT ACE_ROOT);
use lib "$DDS_ROOT/bin";
use lib "$ACE_ROOT/bin";
use PerlDDS::Run_Test;
use File::Compare;
use strict;

my $opendds_idl = PerlDDS::get_opendds_idl();
if (!defined($opendds_idl)) {
    exit 1;
}

my $gencmd = $opendds_idl . " -Sa -St sample.idl";
system($gencmd) == 0 or die "ERROR: could not run $gencmd $?\n";
rename("sampleTypeSupportImpl.cpp", "sampleTy1");
system($gencmd) == 0 or die "ERROR: could not run $gencmd $?\n";
my $result = compare("sampleTy1", "sampleTypeSupportImpl.cpp");
print "test " . ($result == 0 ? "PASSED" : "FAILED") . "\n";
my @intfiles = <sampleTy*>;
unlink(@intfiles) == @intfiles or die "ERROR: could not clean up intermediate files\n";

exit $result;

