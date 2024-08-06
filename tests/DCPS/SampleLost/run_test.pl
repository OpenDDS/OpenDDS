eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# -*- perl -*-

use Env (DDS_ROOT);
use lib "$DDS_ROOT/bin";
use Env (ACE_ROOT);
use lib "$ACE_ROOT/bin";
use PerlDDS::Run_Test;
use File::Path;
use strict;

PerlDDS::add_lib_path('../ConsolidatedMessengerIdl');

my $test = new PerlDDS::TestFramework();
$test->setup_discovery();

my $pubsub_opts = "-DCPSConfigFile pubsub.ini";

while (scalar @ARGV) {
  if ($ARGV[0] =~ /^-d/i) {
    shift;
    $pubsub_opts .= " -DCPSTransportDebugLevel 6 -DCPSDebugLevel 10";
  }
}

$test->process('pubsub', 'pubsub', $pubsub_opts);

rmtree './DCS';

$test->start_process('pubsub');

exit $test->finish(30);
