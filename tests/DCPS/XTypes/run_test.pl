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
  "test|t=s" => \$test_name,
);

my @common_args = ('-DCPSConfigFile rtps_disc.ini -ORBDebugLevel 1 -DCPSDebugLevel 6');
if ($verbose) {
  push(@common_args, "--verbose");
}

my %params = (
  "FirstTest"                             => {reader_type => "Property_1", writer_type => "Property_1", expect_to_fail => 0},
  "SecondTest"                            => {reader_type => "Property_2", writer_type => "Property_2", expect_to_fail => 0},
  "AppendablePass"                        => {reader_type => "AppendableStruct", writer_type => "AppendableStruct", expect_to_fail => 0, reg_type => "AppendableStructT"},
  "AdditionalPrefixFieldStruct"           => {reader_type => "AdditionalPrefixFieldStruct", writer_type => "AdditionalPrefixFieldStruct", expect_to_fail => 0, reg_type => "AdditionalPrefixFieldStructT"},
  "ModifiedMutableStruct"                 => {reader_type => "ModifiedMutableStruct", writer_type => "ModifiedMutableStruct", expect_to_fail => 0, reg_type => "ModifiedMutableStructT"},
  "MutableStruct"                         => {reader_type => "MutableStruct", writer_type => "MutableStruct", expect_to_fail => 0, reg_type => "MutableStructT"},
  "MutableUnion"                          => {reader_type => "MutableUnion", writer_type => "MutableUnion", expect_to_fail => 0, reg_type => "MutableUnionT"},
  "ModifiedMutableUnion"                  => {reader_type => "ModifiedMutableUnion", writer_type => "ModifiedMutableUnion", expect_to_fail => 0, reg_type => "ModifiedMutableUnionT"},
);


sub run_test {
  my $v = $_[0];
  my $test_name_param = $_[1];

  if ($v->{reg_type}) {
    push(@common_args, "--type_r $v->{reg_type}");
  }

  if ($v->{expect_to_fail}) {
    push(@common_args, "--expect_to_fail");
  }


  my @reader_args = ("-ORBLogFile publisher_$test_name_param.log --reader --type $v->{reader_type}");
  push(@reader_args, @common_args);
  $test->process("reader_$test_name_param", 'XTypes', join(' ', @reader_args));
  $test->start_process("reader_$test_name_param");

  my @writer_args = ("-ORBLogFile subscriber_$test_name_param.log --writer --type $v->{writer_type}");
  push(@writer_args, @common_args);
  $test->process("writer_$test_name_param", 'XTypes', join(' ', @writer_args));
  $test->start_process("writer_$test_name_param");
}

if ($test_name eq '') {
  while (my ($k, $v) = each %params) {
    run_test ($v, $k);
    sleep 10;
  }
}
else {
  run_test ($params{$test_name}, $test_name);
}

exit $test->finish(60);
