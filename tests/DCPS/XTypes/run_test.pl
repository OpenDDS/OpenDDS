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

my $secure = 0;
my $verbose = 0;
my $test_name = "";
my $type_consistency = 0;
GetOptions(
  "secure" => \$secure,
  "verbose" => \$verbose,
  "test|t=s" => \$test_name,
  "type-consistency" => \$type_consistency
);

my @common_args = ('-ORBDebugLevel 1 -DCPSDebugLevel 6');
if ($verbose) {
  push(@common_args, "--verbose");
}

my %params;
if ($secure) {
  %params = (
    "FinalStructMatch_s"      => {reader_type => "FinalStructSub", writer_type => "FinalStructPub",
                                  expect_to_fail => 0, reg_type => "PD_OL_OA_OM_OD", key_val => 2,
                                  r_ini => "rtps_disc_security.ini", w_ini => "rtps_disc_security.ini"},
    "AppendableMatch_s"       => {reader_type => "AppendableStruct", writer_type => "AdditionalPostfixFieldStruct",
                                  expect_to_fail => 0, reg_type => "PD_OL_OA_OM_OD", key_val => 4,
                                  r_ini => "rtps_disc_security.ini", w_ini => "rtps_disc_security.ini"},
    "AppendableNoMatch_s"     => {reader_type => "AppendableStruct", writer_type => "AdditionalPrefixFieldStruct",
                                  expect_to_fail => 1, reg_type => "PD_OL_OA_OM_OD", key_val => 5,
                                  r_ini => "rtps_disc_security.ini", w_ini => "rtps_disc_security.ini"},
    "Dependency_s"            => {reader_type => "AppendableStruct", writer_type => "AppendableStructWithDependency",
                                  expect_to_fail => 0, reg_type => "PD_OL_OA_OM_OD", key_val => 14,
                                  r_ini => "rtps_disc_security.ini", w_ini => "rtps_disc_security.ini"},
  );

} else {
  %params = (
    "PlainCdr"                => {reader_type => "PlainCdrStruct", writer_type => "PlainCdrStruct",
                                  expect_to_fail => 0, key_val => 1,
                                  r_ini => "rtps_disc.ini", w_ini => "rtps_disc.ini"},
    "FinalStructMatch"        => {reader_type => "FinalStructSub", writer_type => "FinalStructPub",
                                  expect_to_fail => 0, reg_type => "FinalStructT", key_val => 2,
                                  r_ini => "rtps_disc.ini", w_ini => "rtps_disc.ini"},
    "FinalStructNoMatch"      => {reader_type => "FinalStructSub", writer_type => "ModifiedFinalStruct",
                                  expect_to_fail => 1, reg_type => "FinalStructT_F", key_val => 3,
                                  r_ini => "rtps_disc.ini", w_ini => "rtps_disc.ini"},
    "AppendableMatch"         => {reader_type => "AppendableStruct", writer_type => "AdditionalPostfixFieldStruct",
                                  expect_to_fail => 0, reg_type => "AppendableStructT", key_val => 4,
                                  r_ini => "rtps_disc.ini", w_ini => "rtps_disc.ini"},
    "AppendableNoMatch"       => {reader_type => "AppendableStruct", writer_type => "AdditionalPrefixFieldStruct",
                                  expect_to_fail => 1, reg_type => "AppendableStructT_NoMatch", key_val => 5,
                                  r_ini => "rtps_disc.ini", w_ini => "rtps_disc.ini"},
    "MutableStruct"           => {reader_type => "MutableStruct", writer_type => "ModifiedMutableStruct",
                                  expect_to_fail => 0, reg_type => "MutableStructT", key_val => 6,
                                  r_ini => "rtps_disc.ini", w_ini => "rtps_disc.ini"},
    "MutableStructNoMatchId"  => {reader_type => "MutableStruct", writer_type => "ModifiedIdMutableStruct",
                                  expect_to_fail => 1, reg_type => "MutableStructT_NoMatchId", key_val => 7,
                                  r_ini => "rtps_disc.ini", w_ini => "rtps_disc.ini"},
    "MutableStructNoMatchType"=> {reader_type => "MutableStruct", writer_type => "ModifiedTypeMutableStruct",
                                  expect_to_fail => 1, reg_type => "MutableStructT_NoMatchType", key_val => 8,
                                  r_ini => "rtps_disc.ini", w_ini => "rtps_disc.ini"},
    "MutableStructNoMatchName"=> {reader_type => "MutableStruct", writer_type => "ModifiedNameMutableStruct",
                                  expect_to_fail => 1, reg_type => "MutableStructT_NoMatchName", key_val => 9,
                                  r_ini => "rtps_disc.ini", w_ini => "rtps_disc.ini"},
    "MutableUnion"            => {reader_type => "MutableUnion", writer_type => "ModifiedMutableUnion",
                                  expect_to_fail => 0, reg_type => "MutableUnionT", key_val => 10,
                                  r_ini => "rtps_disc.ini", w_ini => "rtps_disc.ini"},
    "MutableUnionNoMatchDisc" => {reader_type => "MutableUnion", writer_type => "ModifiedDiscMutableUnion",
                                  expect_to_fail => 1, reg_type => "MutableUnionT_NoMatchDisc", key_val => 11,
                                  r_ini => "rtps_disc.ini", w_ini => "rtps_disc.ini"},
    "MutableUnionNoMatchType" => {reader_type => "MutableUnion", writer_type => "ModifiedTypeMutableUnion",
                                  expect_to_fail => 1, reg_type => "MutableUnionT_NoMatchType", key_val => 12,
                                  r_ini => "rtps_disc.ini", w_ini => "rtps_disc.ini"},
    "MutableUnionNoMatchName" => {reader_type => "MutableUnion", writer_type => "ModifiedNameMutableUnion",
                                  expect_to_fail => 1, reg_type => "MutableUnionT_NoMatchName", key_val => 13,
                                  r_ini => "rtps_disc.ini", w_ini => "rtps_disc.ini"},
    "Tryconstruct"            => {reader_type => "Trim20Struct", writer_type => "Trim64Struct",
                                  expect_to_fail => 0, reg_type => "TryconstructT", key_val => 0,
                                  r_ini => "rtps_disc.ini", w_ini => "rtps_disc.ini"},
    "Dependency"              => {reader_type => "AppendableStruct", writer_type => "AppendableStructWithDependency",
                                  expect_to_fail => 0, reg_type => "DependencyT", key_val => 14,
                                  r_ini => "rtps_disc.ini", w_ini => "rtps_disc.ini"},

    "Match_no_xtypes_nn"      => {reader_type => "AppendableStructNoXTypes", writer_type => "AppendableStructNoXTypes",
                                  expect_to_fail => 0, reg_type => "AppendableStructT_no_xtypes_nn",
                                  key_val => 4, r_ini => "rtps_disc_no_xtypes.ini", w_ini => "rtps_disc_no_xtypes.ini"},
    "Match_no_xtypes_yn"      => {reader_type => "AppendableStructNoXTypes", writer_type => "AppendableStructNoXTypes",
                                  expect_to_fail => 0, reg_type => "AppendableStructT_no_xtypes_yn",
                                  key_val => 4, r_ini => "rtps_disc_no_xtypes.ini", w_ini => "rtps_disc.ini"},
    "Match_no_xtypes_ny"      => {reader_type => "AppendableStructNoXTypes", writer_type => "AppendableStructNoXTypes",
                                  expect_to_fail => 0, reg_type => "AppendableStructT_no_xtypes_ny",
                                  key_val => 4, r_ini => "rtps_disc.ini", w_ini => "rtps_disc_no_xtypes.ini"},
  );
}

my %type_consistency_params;
if ($type_consistency) {
  %type_consistency_params = (
    "IgnoreMemberNames_MutableStructMatch" => {reader_type => "MutableStruct", writer_type => "ModifiedNameMutableStruct",
                                               expect_to_fail => 0, key_val => 1, r_ini => "rtps_disc.ini", w_ini => "rtps_disc.ini",
                                               reader_args => "--ignore_member_names"},
    "IgnoreMemberNames_MutableUnionMatch" => {reader_type => "MutableUnion", writer_type => "ModifiedNameMutableUnion",
                                              expect_to_fail => 0, key_val => 1, r_ini => "rtps_disc.ini", w_ini => "rtps_disc.ini"}
  );
}

my $status = 0;

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

  my @reader_args = ("-DCPSConfigFile $v->{r_ini} -ORBLogFile subscriber_$test_name_param.log --type $v->{reader_type}");
  push(@reader_args, @test_args);
  $test->process("reader_$test_name_param", './Sub/xtypes_subscriber', join(' ', @reader_args));
  $test->start_process("reader_$test_name_param");

  my @writer_args = ("-DCPSConfigFile $v->{w_ini} -ORBLogFile publisher_$test_name_param.log --type $v->{writer_type}");
  push(@writer_args, @test_args);
  $test->process("writer_$test_name_param", './Pub/xtypes_publisher', join(' ', @writer_args));
  $test->start_process("writer_$test_name_param");

  $status |= $test->wait_kill("reader_$test_name_param", 30);
  $status |= $test->wait_kill("writer_$test_name_param", 30);
}

if ($test_name eq '') {
  while (my ($k, $v) = each %params) {
    run_test($v, $k);
  }

  while (my ($k, $v) = each %type_consistency_params) {
    run_test($v, $k);
  }
} else {
  run_test($params{$test_name}, $test_name);
}

if ($status) {
  print STDERR "ERROR: test failed\n";
}

exit $test->finish(60);
