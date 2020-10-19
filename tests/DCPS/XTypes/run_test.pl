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
  "PlainCdr"                => {reader_type => "PlainCdrStruct", writer_type => "PlainCdrStruct", expect_to_fail => 0, key_val => 1},
  "FinalStructMatch"        => {reader_type => "FinalStruct", writer_type => "FinalStruct", expect_to_fail => 0, reg_type => "FinalStructT", key_val => 2},
  "FinalStructNoMatch"      => {reader_type => "FinalStruct", writer_type => "ModifiedFinalStruct", expect_to_fail => 1, reg_type => "FinalStructT_F", key_val => 3},
  "AppendableMatch"         => {reader_type => "AppendableStruct", writer_type => "AdditionalPostfixFieldStruct", expect_to_fail => 0, reg_type => "AppendableStructT", key_val => 4},
  "AppendableNoMatch"       => {reader_type => "AppendableStruct", writer_type => "AdditionalPrefixFieldStruct", expect_to_fail => 1, reg_type => "AppendableStructT_F", key_val => 5},
  "MutableStruct"           => {reader_type => "MutableStruct", writer_type => "ModifiedMutableStruct", expect_to_fail => 0, reg_type => "MutableStructT", key_val => 6},
  "MutableStructNoMatchId"  => {reader_type => "MutableStruct", writer_type => "ModifiedMutableStruct_IdDiscrepancy", expect_to_fail => 1, reg_type => "MutableStructT_NoMatchId", key_val => 7},
  "MutableStructNoMatchType"=> {reader_type => "MutableStruct", writer_type => "ModifiedMutableStruct_TypeDiscrepancy", expect_to_fail => 1, reg_type => "MutableStructT_NoMatchType", key_val => 8},
  "MutableUnion"            => {reader_type => "MutableUnion", writer_type => "ModifiedMutableUnion", expect_to_fail => 0, reg_type => "MutableUnionT", key_val => 9},
  "MutableUnionNoMatchDisc" => {reader_type => "MutableUnion", writer_type => "ModifiedDiscMutableUnion", expect_to_fail => 1, reg_type => "MutableUnionT_NoMatchDisc", key_val => 10},
  "MutableUnionNoMatchType" => {reader_type => "MutableUnion", writer_type => "ModifiedTypeMutableUnion", expect_to_fail => 1, reg_type => "MutableUnionT_NoMatchType", key_val => 11},
  "MutableUnionNoMatchName" => {reader_type => "MutableUnion", writer_type => "ModifiedNameMutableUnion", expect_to_fail => 1, reg_type => "MutableUnionT_NoMatchName", key_val => 12},
  "Tryconstruct"            => {reader_type => "Trim20Struct", writer_type => "Trim64Struct", expect_to_fail => 0, reg_type => "TryconstructT", key_val => 0},
  "Dependency"              => {reader_type => "AppendableStruct", writer_type => "AppendableStructWithDependency", expect_to_fail => 0, reg_type => "DependencyT", key_val => 13},
);


sub run_test {
  my @test_args;
  push(@test_args, @common_args);

  my $v = $_[0];
  my $test_name_param = $_[1];

  if ($v->{reg_type}) {
    push(@test_args, "--type_r $v->{reg_type}");
  }

  if ($v->{expect_to_fail} > 0) {
    push(@test_args, "--expect_to_fail");
  }

  if ($v->{key_val} >= 0) {
    push(@test_args, "--key_val $v->{key_val}");
  }

  my @reader_args = ("-ORBLogFile subscriber_$test_name_param.log --reader --type $v->{reader_type}");
  push(@reader_args, @test_args);
  $test->process("reader_$test_name_param", 'XTypes', join(' ', @reader_args));
  $test->start_process("reader_$test_name_param");

  my @writer_args = ("-ORBLogFile publisher_$test_name_param.log --writer --type $v->{writer_type}");
  push(@writer_args, @test_args);
  $test->process("writer_$test_name_param", 'XTypes', join(' ', @writer_args));
  $test->start_process("writer_$test_name_param");
}

if ($test_name eq '') {
  while (my ($k, $v) = each %params) {
    run_test ($v, $k);
    sleep(1);
  }
}
else {
  run_test ($params{$test_name}, $test_name);
}

exit $test->finish(60);
