eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
     & eval 'exec perl -S $0 $argv:q'
     if 0;

use lib "$ENV{ACE_ROOT}/bin";
use lib "$ENV{DDS_ROOT}/bin";
use PerlDDS::Run_Test;
use Getopt::Long;
use strict;

my $test = new PerlDDS::TestFramework();
$test->enable_console_logging();

my $verbose = 0;
GetOptions(
  "verbose" => \$verbose,
);

my @common_args = ('-DCPSConfigFile', 'rtps_disc.ini');
push(@common_args, "--verbose") if ($verbose);

my @reader_args = ('--reader', '--type Property');
push(@reader_args, @common_args);
$test->process('reader1', 'XTypes', join(' ', @reader_args));
$test->start_process('reader1');

my @writer_args = ('--writer', '--type Property');
push(@writer_args, @common_args);
$test->process('writer1', 'XTypes', join(' ', @writer_args));
$test->start_process('writer1');

my @reader_args = ('--reader', '--type Property_2');
push(@reader_args, @common_args);
$test->process('reader2', 'XTypes', join(' ', @reader_args));
$test->start_process('reader2');

my @writer_args = ('--writer', '--type Property_2');
push(@writer_args, @common_args);
$test->process('writer2', 'XTypes', join(' ', @writer_args));
$test->start_process('writer2');

exit $test->finish(60);
