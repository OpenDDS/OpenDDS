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

PerlDDS::add_lib_path('../FooType4');
PerlDDS::add_lib_path('../common');
# single reader with single instances test
my $sub_addr = "localhost:16701";
my $pub_addr = "localhost:";
my $port=29804;
my $is_rtps_disc = 0;
my $DCPScfg = "";
my $DCPSREPO;
my $level = 4;

my $dcpsrepo_ior = "repo.ior";

unlink $dcpsrepo_ior;

if ($ARGV[0] eq "rtps_disc") {
  $DCPScfg = "-DCPSConfigFile " . $ARGV[0] . ".ini ";
  $is_rtps_disc = 1;
  shift;
} elsif ($ARGV[0] eq "rtps_disc_tcp") {
  $DCPScfg = "-DCPSConfigFile " . $ARGV[0] . ".ini ";
  $is_rtps_disc = 1;
  shift;
}

sub run_compatibility_tests {
  return if $status;
  # test multiple cases
  my $compatibility = shift;

  my $pub_durability_kind = shift;
  my $pub_liveliness_kind = shift;
  my $pub_lease_time = shift;
  my $pub_reliability_kind = shift;

  my $sub_durability_kind = shift;
  my $sub_liveliness_kind = shift;
  my $sub_lease_time = shift;
  my $sub_reliability_kind = shift;

  print "\n\n"; # Provide some visual relief between test cases.

  my $sub_time = 40;
  my $pub_time = $sub_time;
  my $sub_parameters = "-s $sub_addr -l $sub_lease_time -x $sub_time -c $compatibility -d $sub_durability_kind -k $sub_liveliness_kind -r $sub_reliability_kind -DCPSDebugLevel $level $DCPScfg -";

  my $Subscriber = PerlDDS::create_process ("subscriber", $sub_parameters);

  my $pub_parameters = "-c $compatibility -d $pub_durability_kind -k $pub_liveliness_kind -r $pub_reliability_kind -l $pub_lease_time -x $pub_time -ORBDebugLevel $level -p $pub_addr$port $DCPScfg";

  my $Publisher = PerlDDS::create_process ("publisher", "$pub_parameters");

  print $Subscriber->CommandLine() . "\n";
  my $SubscriberResult = $Subscriber->Spawn();
  print "Subscriber PID: " . $Subscriber->{PROCESS} . "\n" if $Subscriber->{PROCESS};

  if ($SubscriberResult != 0) {
    print STDERR "ERROR: subscriber returned $SubscriberResult \n";
    $status = 1;
  }

  sleep 3;

  print $Publisher->CommandLine() . "\n";
  my $PublisherResult = $Publisher->SpawnWaitKill ($pub_time + 20);
  print "Publisher PID: " . $Publisher->{PROCESS} . "\n" if $Publisher->{PROCESS};

  if ($PublisherResult != 0) {
    print STDERR "ERROR: publisher returned $PublisherResult \n";
    $status = 1;
  }

  $SubscriberResult = $Subscriber->WaitKill($sub_time + 20);

  if ($SubscriberResult != 0) {
    print STDERR "ERROR: subscriber returned $SubscriberResult \n";
    $status = 1;
  }

}

if (!$is_rtps_disc) {
  $DCPSREPO = PerlDDS::create_process ("$ENV{DDS_ROOT}/bin/DCPSInfoRepo",
                                       "-o $dcpsrepo_ior");

  print $DCPSREPO->CommandLine() . "\n";
  $DCPSREPO->Spawn ();
  print "Repository PID: " . $DCPSREPO->{PROCESS} . "\n" if $DCPSREPO->{PROCESS};

  if (PerlACE::waitforfile_timed ($dcpsrepo_ior, 30) == -1) {
    print STDERR "ERROR: waiting for DCPSInfo IOR file\n";
    $DCPSREPO->Kill ();
    exit 1;
  }
}

run_compatibility_tests("true", "transient_local", "automatic", "5", "reliable", "transient_local", "automatic", "5", "reliable");
run_compatibility_tests("true", "transient_local", "automatic", "6", "reliable", "volatile", "automatic", "7", "best_effort");
run_compatibility_tests("true", "transient_local", "automatic", "5", "reliable", "volatile", "automatic", "infinite", "best_effort");
run_compatibility_tests("true", "transient_local", "automatic", "infinite", "reliable", "volatile", "automatic", "infinite", "best_effort");

run_compatibility_tests("false", "transient_local", "automatic", "6", "reliable", "transient_local", "automatic", "5", "reliable");
run_compatibility_tests("false", "transient_local", "automatic", "infinite", "reliable", "transient_local", "automatic", "5", "reliable");
run_compatibility_tests("false", "transient_local", "automatic", "6", "reliable", "transient_local", "automatic", "5", "reliable");
run_compatibility_tests("false", "transient_local", "automatic", "5", "best_effort", "transient_local", "automatic", "5", "reliable");
run_compatibility_tests("false", "volatile", "automatic", "5", "reliable", "transient_local", "automatic", "5", "reliable");
# there are more to test later, but they are currently treated as invalid
# since there are not supported in the code


if (!$is_rtps_disc) {
  my $ir = $DCPSREPO->TerminateWaitKill(5);

  if ($ir != 0) {
    print STDERR "ERROR: DCPSInfoRepo returned $ir\n";
    $status = 1;
  }
}
if ($status == 0) {
  print "test PASSED.\n";
}
else {
  print STDERR "test FAILED.\n";
}

exit $status;
