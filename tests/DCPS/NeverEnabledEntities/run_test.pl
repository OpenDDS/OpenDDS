eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
     & eval 'exec perl -S $0 $argv:q'
     if 0;

use lib "$ENV{ACE_ROOT}/bin";
use lib "$ENV{DDS_ROOT}/bin";
use PerlDDS::Run_Test;
use strict;

my $test = new PerlDDS::TestFramework();
$test->{add_transport_config} = 0;
$test->process('test', 'NeverEnabledEntities', '-DCPSConfigFile rtps.ini');
$test->start_process('test');
$test->ignore_error('Entity is not enabled.');
exit $test->finish(120);
