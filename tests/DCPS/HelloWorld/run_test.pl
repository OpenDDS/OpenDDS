eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# -*- perl -*-

use Env (DDS_ROOT);
use lib "$DDS_ROOT/bin";
use Env (ACE_ROOT);
use lib "$ACE_ROOT/bin";
use Getopt::Long;
use PerlDDS::Run_Test;
use strict;

my $rtps = 0;
GetOptions("rtps" => \$rtps);

my $common_opts = "";
if ($rtps) {
  my $ini = $ENV{OPENDDS_RTPS_DISCOVERY_MULTICAST_VIA_LOOPBACK} ?
      'rtps_loopback.ini' : 'rtps.ini';
  $common_opts .= "-DCPSConfigFile $ini";
}

my $test = new PerlDDS::TestFramework();
$test->setup_discovery() unless $rtps;

$test->process('subscriber', 'subscriber', $common_opts);
$test->process('publisher', 'publisher', $common_opts);

$test->start_process('publisher');
$test->start_process('subscriber');

exit($test->finish(60));
