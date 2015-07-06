eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
     & eval 'exec perl -S $0 $argv:q'
     if 0;

# -*- perl -*-

use Env (DDS_ROOT);
use lib "$DDS_ROOT/bin";
use Env (ACE_ROOT);
use lib "$ACE_ROOT/bin";
use PerlDDS::Run_Test;

$status = 0;
my $debugFile;
my $debug;
# $debug = 10;
$debugFile = "test.log";
my $debugOpts = "";
$debugOpts .= "-DCPSDebugLevel $debug " if $debug;

unlink $debugFile;

$is_rtps_disc = 1;

my $pub_opts = ($is_rtps_disc == 1 ? "-DCPSConfigFile rtps_disc.ini" : "");

my $opts_1 = $pub_opts . " -x sample_rejected.xml";

$CL1 = PerlDDS::create_process ("inconsistent_qos",
                                "$debugOpts $opts_1");

print $CL1->CommandLine() . "\n";

$client1 = $CL1->SpawnWaitKill (30);

if ($client1 != 0) {
    print STDERR "ERROR: client returned $client1\n";
    $status = 1;
}

if ($status == 0) {
  print "test 1 PASSED.\n";
} else {
  print STDERR "test 1 FAILED.\n";
}

my $opts_2 = $pub_opts . " -x deadline.xml";

$CL2 = PerlDDS::create_process ("inconsistent_qos",
                                "$debugOpts $opts_2");

print $CL2->CommandLine() . "\n";

$client2 = $CL2->SpawnWaitKill (30);

if ($client2 != 0) {
    print STDERR "ERROR: client returned $client2\n";
    $status = 1;
}

if ($status == 0) {
  print "test 2 PASSED.\n";
} else {
  print STDERR "test 2 FAILED.\n";
}

exit $status;
