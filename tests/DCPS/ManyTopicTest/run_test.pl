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

my $status = 0;

PerlDDS::add_lib_path('../ManyTopicTypes');
PerlDDS::add_lib_path('../common');

# single reader with single instances test

my $rtps_disc = 0;

if ($ARGV[0] eq 'rtps') {
  $rtps_disc = 1;
}

my $sub_parameters = "-t all";
my $pub_parameters = "-t all";

my $test = new PerlDDS::TestFramework();
if ($rtps_disc) {
  $sub_parameters .= ' -DCPSConfigFile rtps_disc.ini';
  $pub_parameters .= ' -DCPSConfigFile rtps_disc.ini';
}
else {
  $test->setup_discovery();
}

$test->process("subscriber1", "subscriber", $sub_parameters . " -a reader1");
$test->process("subscriber2", "subscriber", $sub_parameters . " -a reader2");
$test->process("publisher", "publisher", $pub_parameters);

rmtree('./DCS');

$test->start_process("publisher");
$test->start_process("subscriber1");
$test->start_process("subscriber2");

exit $test->finish(300);
