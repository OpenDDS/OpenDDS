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

PerlDDS::add_lib_path('../../Utils/');
PerlDDS::add_lib_path('../FooType4');

my $status = 0;

my $logging_p = "-DCPSDebugLevel 1 -ORBVerboseLogging 1 " .
    "-DCPSTransportDebugLevel 1";
my $logging_s = "-DCPSDebugLevel 1 -ORBVerboseLogging 1 " .
    "-DCPSTransportDebugLevel 1";
my $reliable = 1;

my $nobit = 1; # Set to a non-zero value to disable the Builtin Topics.

my $pub_ini = ' -DCPSConfigFile tcp.ini';
my $sub_ini = ' -DCPSConfigFile tcp.ini';

for my $arg (@ARGV) {
    if ($arg eq 'udp') {
        $pub_ini = " -DCPSConfigFile udp.ini";
        $sub_ini = " -DCPSConfigFile udp.ini";
        $reliable = 0;
    }
    elsif ($arg eq 'multicast') {
        $pub_ini = " -DCPSConfigFile multicast.ini";
        $sub_ini = " -DCPSConfigFile multicast.ini";
    }
    elsif ($arg eq 'multicast_async') {
        $pub_ini = " -DCPSConfigFile pub_multicast_async.ini";
        $sub_ini = " -DCPSConfigFile multicast.ini";
    }
    elsif ($arg eq 'shmem') {
        $pub_ini = " -DCPSConfigFile shmem.ini";
        $sub_ini = " -DCPSConfigFile shmem.ini";
    }
    elsif ($arg eq 'rtps') {
        $pub_ini = " -DCPSConfigFile rtps.ini";
        $sub_ini = " -DCPSConfigFile rtps.ini";
    }
    elsif ($arg eq 'rtps') {
        $pub_ini = " -DCPSConfigFile rtps.ini";
        $sub_ini = " -DCPSConfigFile rtps.ini";
    }
    elsif ($arg eq 'BIT' || $arg eq 'bit') {
        $nobit = 0;
    }
    elsif ($arg ne '') {
        print STDERR "ERROR: invalid test case\n";
        exit 1;
    }
}

my $app_bit_opt = '';
my $repo_bit_opt = '';

$app_bit_opt = '-DCPSBit 0 ' if $nobit;
$repo_bit_opt = '-NOBITS' if $nobit;

my $pub_opts = "$app_bit_opt $pub_ini";
my $sub_opts = "$app_bit_opt $sub_ini";
$sub_opts .= " -r $reliable";
my $pub1_opts = "$logging_p -ORBLogFile pub1.log $pub_opts";
my $sub1_opts = "$logging_s -ORBLogFile sub1.log $sub_opts";

my $dcpsrepo_ior = "repo.ior";

unlink $dcpsrepo_ior;
unlink <*.log>;

my $DCPSREPO = PerlDDS::create_process("$ENV{DDS_ROOT}/bin/DCPSInfoRepo",
                                       "$repo_bit_opt -o $dcpsrepo_ior");

my $Subscriber1 = PerlDDS::create_process("subscriber", $sub1_opts);
my $Publisher1 = PerlDDS::create_process("publisher", $pub1_opts);

print $DCPSREPO->CommandLine() . "\n";
$DCPSREPO->Spawn();
if (PerlACE::waitforfile_timed($dcpsrepo_ior, 30) == -1) {
    print STDERR "ERROR: waiting for Info Repo IOR file\n";
    $DCPSREPO->Kill();
    exit 1;
}

print $Publisher1->CommandLine() . "\n";
$Publisher1->Spawn();

print $Subscriber1->CommandLine() . "\n";
$Subscriber1->Spawn();

my $Subscriber1Result = $Subscriber1->WaitKill(65);
if ($Subscriber1Result != 0) {
    print STDERR "ERROR: subscriber returned $Subscriber1Result\n";
    $status = 1;
}

my $Publisher1Result = $Publisher1->WaitKill(10);
if ($Publisher1Result != 0) {
    print STDERR "ERROR: publisher #1 returned $Publisher1Result\n";
    $status = 1;
}

my $ir = $DCPSREPO->TerminateWaitKill(10);
if ($ir != 0) {
    print STDERR "ERROR: DCPSInfoRepo returned $ir\n";
    $status = 1;
}

unlink $dcpsrepo_ior;

if ($status == 0) {
  print "test PASSED.\n";
} else {
  print "**** Begin log file output *****\n";
  if (open FILE, "<", "pub.log") {
      print "Publisher1:\n";
      while (my $line = <FILE>) {
          print "$line";
      }
      print "\n\n";
      close FILE;
  }

  if (open FILE, "<", "pub2.log") {
      print "Publisher2:\n";
      while (my $line = <FILE>) {
          print "$line";
      }
      print "\n\n";
      close FILE;
  }

  if (open FILE, "<", "sub.log") {
      print "Subscriber:\n";
      while (my $line = <FILE>) {
          print "$line";
      }
      print "\n\n";
      close FILE;
  }

  print "**** End log file output *****\n";

  print STDERR "test FAILED.\n";
}

exit $status;
