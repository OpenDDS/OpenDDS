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
my $debuglevel = 0;

my $pub_opts = "-ORBDebugLevel $debuglevel -DCPSConfigFile pub.ini -DCPSDebugLevel $debuglevel -DCPSBits 0";
my $sub_opts = "-DCPSTransportDebugLevel $debuglevel -ORBDebugLevel $debuglevel -DCPSConfigFile sub.ini -DCPSDebugLevel $debuglevel -DCPSBits 0";
my $testcase = "";

if ($ARGV[0] eq 'group') {
  $testcase = "-q 2"; #default
}
elsif ($ARGV[0] eq 'topic') {
  $testcase = "-q 1";
}
elsif ($ARGV[0] eq 'instance') {
  $testcase = "-q 0";
}
elsif ($ARGV[0] ne '') {
  print STDERR "ERROR: invalid parameter $ARGV[0]\n";
  exit 1;
}

my $dcpsrepo_ior = "repo.ior";

unlink $dcpsrepo_ior;
unlink <*.log>;

my $test = new PerlDDS::TestFramework();

$test->setup_discovery("-ORBDebugLevel 1 -ORBLogFile DCPSInfoRepo.log ");

$test->process("subscriber", "subscriber", "$sub_opts -ORBVerboseLogging 1 $testcase");
$test->process("publisher", "publisher", $pub_opts);

$test->start_process("subscriber");
$test->start_process("publisher");

exit $test->finish(120);
