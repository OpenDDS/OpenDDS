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
my $test_name = "";
GetOptions(
  "verbose" => \$verbose,
  "test|f=s"   => \$test_name,
);

my $common_args = ('-DCPSConfigFile rtps_disc.ini -ORBDebugLevel 1 -DCPSDebugLevel 6');
if ($verbose) {
    $common_args = ("$common_args, --verbose");
}

my %params = (
  "FirstTest"                             => {reader_type => "Property_1", writer_type => "Property_1", expect_to_fail => ""},
  "SecondTest"                            => {reader_type => "Property_2", writer_type => "Property_2", expect_to_fail => ""},
);

if ($test_name eq '') {
  while (my ($k, $v) = each %params) {
    my @reader_args = ("$common_args -ORBLogFile publisher_$k.log --reader --type $v->{reader_type} $v->{expect_to_fail}");
    $test->process("reader_$k", 'XTypes', join(' ', @reader_args));
    $test->start_process("reader_$k");

    my @writer_args = ("$common_args -ORBLogFile subscriber_$k.log --writer --type $v->{writer_type} $v->{expect_to_fail}");
    $test->process("writer_$k", 'XTypes', join(' ', @writer_args));
    $test->start_process("writer_$k");

    sleep 5;
  }
} else {
  my $v = $params{$test_name};
 
  my @reader_args = ("$common_args -ORBLogFile publisher_$test_name.log --reader --type $v->{reader_type} $v->{expect_to_fail}");
  $test->process("reader_$test_name", 'XTypes', join(' ', @reader_args));
  $test->start_process("reader_$test_name");

  my @writer_args = ("$common_args -ORBLogFile subscriber_$test_name.log --writer --type $v->{writer_type} $v->{expect_to_fail}");
  $test->process("writer_$test_name", 'XTypes', join(' ', @writer_args));
  $test->start_process("writer_$test_name");
}

exit $test->finish(60);
