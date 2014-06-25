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

my $logging_p = "-DCPSDebugLevel 1 -ORBVerboseLogging 1 " .
    "-DCPSTransportDebugLevel 1";#6 -DCPSDebugLevel 10";
my $logging_s = "-DCPSDebugLevel 1 -ORBVerboseLogging 1 " .
    "-DCPSTransportDebugLevel 1";#6 -DCPSDebugLevel 10";
my $pub_opts = "$logging_p -ORBLogFile pub.log ";
my $sub_opts = "$logging_s -ORBLogFile sub.log ";
my $repo_bit_opt = '';
my $reliable = 1;

my $nobit = 1; # Set to a non-zero value to disable the Builtin Topics.
my $app_bit_opt = '-DCPSBit 0 ' if $nobit;
$repo_bit_opt = '-NOBITS' if $nobit;


if ($ARGV[0] eq 'udp') {
    $pub_opts .= "$app_bit_opt -DCPSConfigFile udp.ini";
    $sub_opts .= "$app_bit_opt -DCPSConfigFile udp.ini";
    $reliable = 0;
}
elsif ($ARGV[0] eq 'multicast') {
    $pub_opts .= "$app_bit_opt -DCPSConfigFile multicast.ini";
    $sub_opts .= "$app_bit_opt -DCPSConfigFile multicast.ini";
}
elsif ($ARGV[0] eq 'multicast_async') {
    $pub_opts .= "$app_bit_opt -DCPSConfigFile pub_multicast_async.ini";
    $sub_opts .= "$app_bit_opt -DCPSConfigFile multicast.ini";
}
elsif ($ARGV[0] eq 'shmem') {
    $pub_opts .= "$app_bit_opt -DCPSConfigFile shmem.ini";
    $sub_opts .= "$app_bit_opt -DCPSConfigFile shmem.ini";
}
elsif ($ARGV[0] eq 'rtps') {
    $pub_opts .= '$app_bit_opt -DCPSConfigFile rtps.ini';
    $sub_opts .= '$app_bit_opt -DCPSConfigFile rtps.ini';
}
elsif ($ARGV[0] ne '') {
    print STDERR "ERROR: invalid test case\n";
    exit 1;
}
else {
    $pub_opts .= ' -DCPSConfigFile tcp.ini';
    $sub_opts .= ' -DCPSConfigFile tcp.ini';
}

my $dcpsrepo_ior = "repo.ior";

unlink $dcpsrepo_ior;
unlink <*.log>;

my $DCPSREPO = PerlDDS::create_process("$ENV{DDS_ROOT}/bin/DCPSInfoRepo",
                                       "$repo_bit_opt -o $dcpsrepo_ior");
$sub_opts .= " -r $reliable";

my $Subscriber = PerlDDS::create_process("subscriber", $sub_opts);
my $Publisher = PerlDDS::create_process("publisher", $pub_opts);

my $pub2_opts = $pub_opts;
$pub2_opts =~ s/pub\.log/pub2.log/;
my $Publisher2 = PerlDDS::create_process("publisher", $pub2_opts);

print $DCPSREPO->CommandLine() . "\n";
$DCPSREPO->Spawn();
if (PerlACE::waitforfile_timed($dcpsrepo_ior, 30) == -1) {
    print STDERR "ERROR: waiting for Info Repo IOR file\n";
    $DCPSREPO->Kill();
    exit 1;
}

print $Publisher->CommandLine() . "\n";
$Publisher->Spawn();

print $Publisher2->CommandLine() . "\n";
$Publisher2->Spawn();

print $Subscriber->CommandLine() . "\n";
$Subscriber->Spawn();

$status |= PerlDDS::wait_kill($Subscriber, 65, "subscriber");

$status |= PerlDDS::wait_kill($Publisher, 10, "publisher #1");
$status |= PerlDDS::wait_kill($Publisher2, 10, "publisher #2");
$status |= PerlDDS::terminate_wait_kill($DCPSREPO);

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
