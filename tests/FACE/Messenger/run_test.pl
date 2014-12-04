eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

use Env (DDS_ROOT);
use lib "$DDS_ROOT/bin";
use Env (ACE_ROOT);
use lib "$ACE_ROOT/bin";
use PerlDDS::Run_Test;
use strict;

my $status = 0;

my $dcpsrepo_ior = "repo.ior";
unlink $dcpsrepo_ior;
my $DCPSREPO = PerlDDS::create_process("$ENV{DDS_ROOT}/bin/DCPSInfoRepo");

PerlDDS::add_lib_path("Idl");
my $Subscriber = PerlDDS::create_process("Subscriber/subscriber");
my $Publisher = PerlDDS::create_process("Publisher/publisher");

$DCPSREPO->Spawn();
if (PerlACE::waitforfile_timed($dcpsrepo_ior, 30) == -1) {
  print STDERR "ERROR: waiting for Info Repo IOR file\n";
  $DCPSREPO->Kill();
  exit 1;
}

$Subscriber->Spawn();
sleep 5;

$Publisher->Spawn();
my $PublisherResult = $Publisher->WaitKill(30);
if ($PublisherResult != 0) {
  print STDERR "ERROR: publisher returned $PublisherResult\n";
  $status = 1;
}

my $SubscriberResult = $Subscriber->WaitKill(30);
if ($SubscriberResult != 0) {
  print STDERR "ERROR: subscriber returned $SubscriberResult\n";
  $status = 1;
}

my $ir = $DCPSREPO->TerminateWaitKill(5);
if ($ir != 0) {
  print STDERR "ERROR: DCPSInfoRepo returned $ir\n";
  $status = 1;
}

unlink $dcpsrepo_ior;

exit $status;
