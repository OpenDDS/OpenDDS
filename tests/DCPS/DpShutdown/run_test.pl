eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
     & eval 'exec perl -S $0 $argv:q'
     if 0;

use strict;
use warnings;

use lib "$ENV{DDS_ROOT}/bin";
use lib "$ENV{ACE_ROOT}/bin";
use PerlDDS::Run_Test;

PerlDDS::add_lib_path('../ConsolidatedMessengerIdl');

my $status = 0;

my $test = new PerlDDS::TestFramework();
$test->{add_transport_config} = 0;

my @args = ();
my $value;
if ($test->flag('t', \$value)) {
  push(@args, '-t', $value);
}

$test->process("dpshutdown", "dpshutdown", join(' ', @args));
$test->start_process("dpshutdown");
my $client = $test->finish(30);
if ($client != 0) {
  print STDERR "ERROR: client returned $client\n";
  $status = 1;
}

exit $status;
