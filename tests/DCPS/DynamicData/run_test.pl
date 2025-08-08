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

my $test = new PerlDDS::TestFramework();
$test->setup_discovery();

my $dyn;
$test->flag('dyn', \$dyn);
my $adapt = 0;
if ($test->flag('adapt')) {
  $adapt = 1;
}

my @pub_args = ();
push(@pub_args, "-dynamic") if ($dyn eq 'dw');
push(@pub_args, "-adapter") if ($adapt);

$test->process('subscriber', 'subscriber', $dyn eq 'dr' ? '-dynamic' : '');
$test->process('publisher', 'publisher', join(' ', @pub_args));

$test->start_process('publisher');
$test->start_process('subscriber');

exit $test->finish(30);
