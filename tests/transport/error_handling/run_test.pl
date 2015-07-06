eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
     & eval 'exec perl -S $0 $argv:q'
     if 0;

# -*- perl -*-

use Env (DDS_ROOT);
use lib "$DDS_ROOT/bin";
use Env (ACE_ROOT);
use lib "$ACE_ROOT/bin";
use PerlDDS::Run_Test;
use strict;

my $status = 0;
my $testoutputfilename = "test.log";

unlink $testoutputfilename;

my $main = PerlDDS::create_process("main");

# Redirect stderr to a file so expected ERRORs won't show
# up in output indicate test has failed.
open(SAVEERR, ">&STDERR");
open(STDERR, ">$testoutputfilename") || die "ERROR: Can't redirect stderr";

$main->Spawn();

my $testResult = $main->WaitKill(30);
if ($testResult != 0) {
    print STDERR "ERROR: test returned $testResult\n";
    $status = 1;
}

close(STDERR);
open(STDERR, ">&SAVEERR");

if ($status == 0) {
  print "test PASSED.\n";
} else {
  print STDERR "test FAILED.\n";
}

exit $status


