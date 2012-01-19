eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# $Id$
# -*- perl -*-

my @original_ARGV = @ARGV;

use Env (DDS_ROOT);
use lib "$DDS_ROOT/bin";
use Env (ACE_ROOT);
use lib "$ACE_ROOT/bin";
use PerlDDS::Run_Test;
use strict;

my $status = 0;

my $dbg_lvl = '-ORBDebugLevel 1 -DCPSDebugLevel 4 -DCPSTransportDebugLevel 2'; 
my $pub_opts = "$dbg_lvl -ORBLogFile pub.log";
my $sub_opts = "$dbg_lvl -ORBLogFile sub.log";
my $repo_bit_opt = "";
my $stack_based = 0;
my $is_rtps_disc = 0;
my $DCPSREPO;

unlink qw/DCPSInfoRepo.log pub.log sub.log/;

if ($ARGV[0] eq 'udp') {
    $pub_opts .= " -DCPSConfigFile pub_udp.ini";
    $sub_opts .= " -DCPSConfigFile sub_udp.ini";
}
elsif ($ARGV[0] eq 'multicast') {
    $pub_opts .= " -DCPSConfigFile pub_multicast.ini";
    $sub_opts .= " -DCPSConfigFile sub_multicast.ini";
}
elsif ($ARGV[0] eq 'default_tcp') {
    $pub_opts .= " -t tcp";
    $sub_opts .= " -t tcp";
}
elsif ($ARGV[0] eq 'default_udp') {
    $pub_opts .= " -t udp";
    $sub_opts .= " -t udp";
}
elsif ($ARGV[0] eq 'default_multicast') {
    $pub_opts .= " -t multicast";
    $sub_opts .= " -t multicast";
}
elsif ($ARGV[0] eq 'nobits') {
    $repo_bit_opt = '-NOBITS';
    $pub_opts .= ' -DCPSConfigFile pub.ini -DCPSBit 0';
    $sub_opts .= ' -DCPSConfigFile sub.ini -DCPSBit 0';
}
elsif ($ARGV[0] eq 'ipv6') {
    $pub_opts .= " -DCPSConfigFile pub_ipv6.ini";
    $sub_opts .= " -DCPSConfigFile sub_ipv6.ini";
}
elsif ($ARGV[0] eq 'stack') {
    $pub_opts .= " -t tcp";
    $sub_opts .= " -t tcp";
    $stack_based = 1;
}
elsif ($ARGV[0] eq 'rtps') {
    $pub_opts .= " -DCPSConfigFile rtps.ini";
    $sub_opts .= " -DCPSConfigFile rtps.ini";
}
elsif ($ARGV[0] eq 'rtps_disc') {
    $pub_opts .= " -DCPSConfigFile rtps_disc.ini";
    $sub_opts .= " -DCPSConfigFile rtps_disc.ini";
    $is_rtps_disc = 1;
}
elsif ($ARGV[0] eq 'rtps_disc_tcp') {
    $pub_opts .= " -DCPSConfigFile rtps_disc_tcp.ini";
    $sub_opts .= " -DCPSConfigFile rtps_disc_tcp.ini";
    $is_rtps_disc = 1;
}
elsif ($ARGV[0] eq 'rtps_unicast') {
    $repo_bit_opt = '-NOBITS';
    $pub_opts .= " -DCPSConfigFile rtps_uni.ini -DCPSBit 0";
    $sub_opts .= " -DCPSConfigFile rtps_uni.ini -DCPSBit 0";
}
elsif ($ARGV[0] eq 'all') {
    @original_ARGV = grep { $_ ne 'all' } @original_ARGV;
    my @tests = ('', qw/udp multicast default_tcp default_udp default_multicast
                        nobits stack
                        rtps rtps_disc rtps_unicast rtps_disc_tcp/);
    push(@tests, 'ipv6') if new PerlACE::ConfigList->check_config('IPV6');
    for my $test (@tests) {
        $status += system($^X, $0, @original_ARGV, $test);
    }
    exit $status;
}
elsif ($ARGV[0] ne '') {
    print STDERR "ERROR: invalid test case\n";
    exit 1;
}
else {
    $pub_opts .= ' -DCPSConfigFile pub.ini';
    $sub_opts .= ' -DCPSConfigFile sub.ini';
}

my $dcpsrepo_ior = "repo.ior";

unlink $dcpsrepo_ior;

unless ($is_rtps_disc) {
  $DCPSREPO = PerlDDS::create_process("$ENV{DDS_ROOT}/bin/DCPSInfoRepo",
                     "-ORBDebugLevel 1 -ORBLogFile DCPSInfoRepo.log " .
                     "$repo_bit_opt -o $dcpsrepo_ior");
}

my $Subscriber = PerlDDS::create_process(($stack_based ? 'stack_' : '') .
                                         "subscriber", $sub_opts);

my $Publisher = PerlDDS::create_process("publisher", $pub_opts);

unless ($is_rtps_disc) {
  print $DCPSREPO->CommandLine() . "\n";
  $DCPSREPO->Spawn();

  if (PerlACE::waitforfile_timed($dcpsrepo_ior, 30) == -1) {
      print STDERR "ERROR: waiting for DCPSInfo IOR file\n";
      $DCPSREPO->Kill();
      exit 1;
  }
}

print $Publisher->CommandLine() . "\n";
$Publisher->Spawn();

print $Subscriber->CommandLine() . "\n";
$Subscriber->Spawn();


my $PublisherResult = $Publisher->WaitKill(300);
if ($PublisherResult != 0) {
    print STDERR "ERROR: publisher returned $PublisherResult \n";
    $status = 1;
}

my $SubscriberResult = $Subscriber->WaitKill(15);
if ($SubscriberResult != 0) {
    print STDERR "ERROR: subscriber returned $SubscriberResult \n";
    $status = 1;
}

unless ($is_rtps_disc) {
  my $ir = $DCPSREPO->TerminateWaitKill(5);
  if ($ir != 0) {
      print STDERR "ERROR: DCPSInfoRepo returned $ir\n";
      $status = 1;
  }
  unlink $dcpsrepo_ior;
}

if ($status == 0) {
  print "test PASSED.\n";
} else {
  print STDERR "test FAILED.\n";
}

exit $status;
