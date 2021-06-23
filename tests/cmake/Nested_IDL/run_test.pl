eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# -*- perl -*-

my @original_ARGV = @ARGV;

use Env (DDS_ROOT);
use lib "$DDS_ROOT/bin";
use Env (ACE_ROOT);
use lib "$ACE_ROOT/bin";
use PerlDDS::Run_Test;
use strict;

my $status = 0;

my $test = new PerlDDS::TestFramework();

my $test_file_name = "ecu";

$test->process("main", $test_file_name, "");

$test->start_process("main");

# start killing processes in 3 seconds
exit $test->finish(3) ? 1 : 0;
