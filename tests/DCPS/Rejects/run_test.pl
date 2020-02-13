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

PerlDDS::add_lib_path('../ConsolidatedMessengerIdl');

my $test = new PerlDDS::TestFramework();
$test->{'dcps_debug_level'} = $test->{'dcps_transport_debug_level'} = 0;
$test->ignore_error('DataWriterImpl::register_instance_i: register instance ' .
                    'with container failed');
$test->ignore_error('Messenger::MessageDataWriterImpl::register_instance_w_timestamp: register failed');
$test->setup_discovery();
$test->process("sub", "subscriber");
$test->process("pub", "publisher");
$test->start_process("pub");
$test->start_process("sub");

exit $test->finish(300);
