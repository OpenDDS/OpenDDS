eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
     & eval 'exec perl -S $0 $argv:q'
     if 0;

use lib "$ENV{ACE_ROOT}/bin";
use lib "$ENV{DDS_ROOT}/bin";
use PerlDDS::Run_Test;
use File::Compare;
use Getopt::Long;
use strict;

my $test = new PerlDDS::TestFramework();
$test->enable_console_logging();

my $large_samples = 0;
my $verbose = 0;
my $early_reader = 0;
GetOptions(
  "large-samples" => \$large_samples,
  "verbose" => \$verbose,
  "early-reader" => \$early_reader,
);

my @common_args = ('-DCPSConfigFile', 'rtps_disc.ini');
push(@common_args, "--verbose") if ($verbose);
push(@common_args, "--large-samples") if ($large_samples);
push(@common_args, "--has-early-reader") if $early_reader;

my @writer_args = ('--writer');
push(@writer_args, @common_args);

my @reader_args = ('--reader');
push(@reader_args, @common_args);

my $readerAfile = 'readerA.txt';
if ($early_reader) {
  unlink $readerAfile;
  $test->process('readerA', 'DelayedDurable',
                 join(' ', @reader_args, '--report-last-value', $readerAfile));
  $test->start_process('readerA');
}

$test->process('writer', 'DelayedDurable', join(' ', @writer_args));
$test->start_process('writer');

if ($early_reader) {
  if (PerlACE::waitforfile_timed($readerAfile, 30) == -1) {
    print STDERR "ERROR: waiting for reader data file\n";
    $test->finish();
    exit 1;
  }
}
else {
  sleep 15;
}

my $readerBfile = 'readerB.txt';
unlink $readerBfile;
push(@reader_args, '--report-last-value', $readerBfile) if $early_reader;

$test->process('readerB', 'DelayedDurable', join(' ', @reader_args));
$test->start_process('readerB');

my $ret = $test->finish(60);
if ($early_reader) {
  if (compare($readerAfile, $readerBfile)) {
    print STDERR "ERROR: Readers got different last-sample values\n";
    PerlDDS::print_file($readerAfile);
    PerlDDS::print_file($readerBfile);
    exit 1;
  }
  unlink $readerAfile, $readerBfile;
}
exit $ret;
