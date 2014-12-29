eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# $Id$
# -*- perl -*-

use Env (DDS_ROOT);
use lib "$DDS_ROOT/bin";
use Env (ACE_ROOT);
use lib "$ACE_ROOT/bin";
use PerlDDS::Run_Test;
use strict;

my $status = 0;

my $dcpsrepo_ior = "repo.ior";


unlink $dcpsrepo_ior;

# -b
my $parameters = "-DcpsBit 0";
# or could have
# $parameters = "-b -DcpsBit 1";

if ($ARGV[0] eq 'by_instance') {
  $parameters .= " -i";
}

# -ORBDebugLevel 1 -NOBITS
my $DCPSREPO = PerlDDS::create_process("$ENV{DDS_ROOT}/bin/DCPSInfoRepo",
                                    "-o $dcpsrepo_ior"
                                    . " -NOBITS");

my $ZCTest = PerlDDS::create_process("main", $parameters);

if (!$PerlDDS::SafetyProfile && $DCPSREPO->Spawn () != 0) {
    print STDERR "ERROR: Couldn't spawn InfoRepo\ntest FAILED.\n";
    exit 1;
}

if (!$PerlDDS::SafetyProfile && PerlACE::waitforfile_timed ($dcpsrepo_ior, 30) == -1) {
    print STDERR "ERROR: waiting for Info Repo IOR file\n";
    $DCPSREPO->Kill ();
    exit 1;
}

if ($ZCTest->Spawn () != 0) {
    print STDERR "ERROR: Couldn't spawn main\ntest FAILED.\n";
    exit 1;
}

my $result = $ZCTest->WaitKill(60);

if ($result != 0) {
    print STDERR "ERROR: main returned $result \n";
    $status = 1;
}

my $ir = 0;
$DCPSREPO->TerminateWaitKill(5) unless $PerlDDS::SafetyProfile;

if ($ir != 0) {
    print STDERR "ERROR: DCPSInfoRepo returned $ir\n";
    $status = 1;
}

if ($status == 0) {
  print "test PASSED.\n";
}
else {
  print STDERR "test FAILED.\n";
}
exit $status;
