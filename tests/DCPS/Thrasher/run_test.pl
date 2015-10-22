eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# -*- perl -*-

use strict;
use warnings;

use Env qw(DDS_ROOT ACE_ROOT);
use lib "$DDS_ROOT/bin";
use lib "$ACE_ROOT/bin";
use PerlDDS::Run_Test;

PerlDDS::add_lib_path('../FooType');

my $pub_opts = "";
my $sub_opts = "";
# $pub_opts .= "-DCPSChunks 1 ";

my $arg = shift || "";
if ($arg eq 'low') {
  $pub_opts .= "-t 8 -s 128";
  $sub_opts .= "-t 8 -n 1024";

} elsif ($arg eq 'medium') {
  $pub_opts .= "-t 16 -s 64";
  $sub_opts .= "-t 16 -n 1024";

} elsif ($arg eq 'high') {
  $pub_opts .= "-t 32 -s 32";
  $sub_opts .= "-t 32 -n 1024";

} elsif ($arg eq 'aggressive') {
  $pub_opts .= "-t 64 -s 16";
  $sub_opts .= "-t 64 -n 1024";

} elsif ($arg eq 'single') {
  $pub_opts .= "-t 1 -s 1";
  $sub_opts .= "-t 1 -n 1";

} else { # default (i.e. lazy)
  $pub_opts .= "-t 1 -s 1024";
  $sub_opts .= "-t 1 -n 1024";
}

my $ini_file = "thrasher.ini";

if (("rtps" eq $arg) || ("rtps" eq (shift || ""))) {
  $ini_file = "thrasher_rtps.ini";
}

$pub_opts .= " -DCPSConfigFile $ini_file";
$sub_opts .= " -DCPSConfigFile $ini_file";

my $test = new PerlDDS::TestFramework();
$test->setup_discovery();
$test->enable_console_logging();

$test->process('sub', 'subscriber', $sub_opts);
$test->process('pub', 'publisher', $pub_opts);

$test->start_process('sub');
$test->start_process('pub');

exit $test->finish(300);
