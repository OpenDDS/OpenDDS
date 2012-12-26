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
my $debug = 0;
# $debug = 10;
my $debugFile = "test.log";
my $debugOpts = "-DCPSDebugLevel $debug" if $debug;

unlink $debugFile;

my $pub_opts = "-DCPSConfigFile rtps_disc.ini";

my $CL = PerlDDS::create_process("tpreuse", "$debugOpts $pub_opts");

print $CL->CommandLine() . "\n";

my $client = $CL->SpawnWaitKill(60);

if ($client != 0) {
    print STDERR "ERROR: client returned $client\n";
    $status = 1;
}

if ($status == 0) {
  print "test PASSED.\n";
} else {
  print STDERR "test FAILED.\n";
}

exit $status;
