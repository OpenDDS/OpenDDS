eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

use strict;
use warnings;

use lib "$ENV{DDS_ROOT}/bin";
use lib "$ENV{ACE_ROOT}/bin";
use PerlDDS::Run_Test;

PerlDDS::add_lib_path("lib");

my $test = new PerlDDS::TestFramework();
$test->process("user", "bin/opendds_install_test_user");
$test->start_process("user");
exit($test->finish(2) ? 1 : 0);
