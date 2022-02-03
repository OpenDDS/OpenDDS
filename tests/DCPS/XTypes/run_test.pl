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
my $tcp = 0;
my $verbose = 0;
my $test_name = "";
GetOptions(
  "secure" => \$secure,
  "tcp" => \$tcp,
  "verbose" => \$verbose,
  "test|t=s" => \$test_name,
);

my @common_args = ('-ORBDebugLevel 1 -DCPSDebugLevel 6');
if ($verbose) {
  push(@common_args, "--verbose");
}

my %params;
if ($secure) {
  %params = (
    "FinalStructMatch_s" => {
      reader_type => "FinalStructSub", writer_type => "FinalStructPub",
      expect_inconsistent_topic => 0, topic => "PD_OL_OA_OM_OD", key_val => 2,
      r_ini => "rtps_disc_security.ini", w_ini => "rtps_disc_security.ini",
    },
    "AppendableMatch_s" => {
      reader_type => "AppendableStruct", writer_type => "AdditionalPostfixFieldStruct",
      expect_inconsistent_topic => 0, topic => "PD_OL_OA_OM_OD", key_val => 4,
      r_ini => "rtps_disc_security.ini", w_ini => "rtps_disc_security.ini",
    },
    "AppendableNoMatch_s" => {
      reader_type => "AppendableStruct", writer_type => "AdditionalPrefixFieldStruct",
      expect_inconsistent_topic => 1, topic => "PD_OL_OA_OM_OD", key_val => 5,
      r_ini => "rtps_disc_security.ini", w_ini => "rtps_disc_security.ini",
    },
    "Dependency_s" => {
      reader_type => "AppendableStruct", writer_type => "AppendableStructWithDependency",
      expect_inconsistent_topic => 0, topic => "PD_OL_OA_OM_OD", key_val => 14,
      r_ini => "rtps_disc_security.ini", w_ini => "rtps_disc_security.ini",
    },
  );

  $test->generate_governance("AU_UA_ND_NL_NR", "governance.xml.p7s");

} elsif ($tcp) {
  %params = (
    "Tcp_MutableUnionNoMatchQos" => {
      reader_type => "MutableUnion", writer_type => "ModifiedMutableUnion",
      expect_inconsistent_topic => 0, topic => "MutableUnionT", key_val => 10,
      r_ini => "tcp.ini", w_ini => "tcp.ini", expect_incompatible_qos => 1
    },
    "Tcp_AppendableMatch" => {
      reader_type => "AppendableStruct", writer_type => "AdditionalPostfixFieldStruct",
      expect_inconsistent_topic => 0, topic => "AppendableStructT", key_val => 4,
      r_ini => "tcp.ini", w_ini => "tcp.ini",
    },
    "Tcp_AppendableNoMatch" => {
      reader_type => "AppendableStruct", writer_type => "AdditionalPrefixFieldStruct",
      expect_inconsistent_topic => 1, topic => "AppendableStructT_NoMatch", key_val => 5,
      r_ini => "tcp.ini", w_ini => "tcp.ini",
    },
    "Tcp_MutableStruct" => {
      reader_type => "MutableStruct", writer_type => "ModifiedMutableStruct",
      expect_inconsistent_topic => 0, topic => "MutableStructT", key_val => 6,
      r_ini => "tcp.ini", w_ini => "tcp.ini",
    },
    "Tcp_MutableBaseStruct" => {
      reader_type => "MutableStruct", writer_type => "MutableBaseStruct",
      expect_inconsistent_topic => 0, topic => "MutableBaseStructT", key_val => 7,
      r_ini => "tcp.ini", w_ini => "tcp.ini",
    },
    "Tcp_MutableStructNoMatchName" => {
      reader_type => "MutableStruct", writer_type => "ModifiedNameMutableStruct",
      expect_inconsistent_topic => 1, topic => "MutableStructT_NoMatchName", key_val => 9,
      r_ini => "tcp.ini", w_ini => "tcp.ini",
    },
    "Tcp_MutableUnion" => {
      reader_type => "MutableUnion", writer_type => "ModifiedMutableUnion",
      expect_inconsistent_topic => 0, topic => "MutableUnionT", key_val => 10,
      r_ini => "tcp.ini", w_ini => "tcp.ini",
    },
    "Tcp_MutableUnionNoMatchDisc" => {
      reader_type => "MutableUnion", writer_type => "ModifiedDiscMutableUnion",
      expect_inconsistent_topic => 1, topic => "MutableUnionT_NoMatchDisc", key_val => 11,
      r_ini => "tcp.ini", w_ini => "tcp.ini",
    },
  );

} else {
  %params = (
    "PlainCdr" => {
      reader_type => "PlainCdrStruct", writer_type => "PlainCdrStruct",
      expect_inconsistent_topic => 0, key_val => 1,
      r_ini => "rtps_disc.ini", w_ini => "rtps_disc.ini",
    },
    "FinalStructMatch" => {
      reader_type => "FinalStructSub", writer_type => "FinalStructPub",
      expect_inconsistent_topic => 0, topic => "FinalStructT", key_val => 2,
      r_ini => "rtps_disc.ini", w_ini => "rtps_disc.ini",
    },
    "FinalStructNoMatch" => {
      reader_type => "FinalStructSub", writer_type => "ModifiedFinalStruct",
      expect_inconsistent_topic => 1, topic => "FinalStructT_F", key_val => 3,
      r_ini => "rtps_disc.ini", w_ini => "rtps_disc.ini",
    },
    "AppendableMatch" => {
      reader_type => "AppendableStruct", writer_type => "AdditionalPostfixFieldStruct",
      expect_inconsistent_topic => 0, topic => "AppendableStructT", key_val => 4,
      r_ini => "rtps_disc.ini", w_ini => "rtps_disc.ini",
    },
    "AppendableNoMatch" => {
      reader_type => "AppendableStruct", writer_type => "AdditionalPrefixFieldStruct",
      expect_inconsistent_topic => 1, topic => "AppendableStructT_NoMatch", key_val => 5,
      r_ini => "rtps_disc.ini", w_ini => "rtps_disc.ini",
    },
    "MutableStruct" => {
      reader_type => "MutableStruct", writer_type => "ModifiedMutableStruct",
      expect_inconsistent_topic => 0, topic => "MutableStructT", key_val => 6,
      r_ini => "rtps_disc.ini", w_ini => "rtps_disc.ini",
    },
    "MutableStructNoMatchId" => {
      reader_type => "MutableStruct", writer_type => "ModifiedIdMutableStruct",
      expect_inconsistent_topic => 1, topic => "MutableStructT_NoMatchId", key_val => 7,
      r_ini => "rtps_disc.ini", w_ini => "rtps_disc.ini",
    },
    "MutableStructNoMatchType" => {
      reader_type => "MutableStruct", writer_type => "ModifiedTypeMutableStruct",
      expect_inconsistent_topic => 1, topic => "MutableStructT_NoMatchType", key_val => 8,
      r_ini => "rtps_disc.ini", w_ini => "rtps_disc.ini",
    },
    "MutableStructNoMatchName" => {
      reader_type => "MutableStruct", writer_type => "ModifiedNameMutableStruct",
      expect_inconsistent_topic => 1, topic => "MutableStructT_NoMatchName", key_val => 9,
      r_ini => "rtps_disc.ini", w_ini => "rtps_disc.ini",
    },
    "MutableUnion" => {
      reader_type => "MutableUnion", writer_type => "ModifiedMutableUnion",
      expect_inconsistent_topic => 0, topic => "MutableUnionT", key_val => 10,
      r_ini => "rtps_disc.ini", w_ini => "rtps_disc.ini",
    },
    "MutableUnionNoMatchDisc" => {
      reader_type => "MutableUnion", writer_type => "ModifiedDiscMutableUnion",
      expect_inconsistent_topic => 1, topic => "MutableUnionT_NoMatchDisc", key_val => 11,
      r_ini => "rtps_disc.ini", w_ini => "rtps_disc.ini",
    },
    "MutableUnionNoMatchType" => {
      reader_type => "MutableUnion", writer_type => "ModifiedTypeMutableUnion",
      expect_inconsistent_topic => 1, topic => "MutableUnionT_NoMatchType", key_val => 12,
      r_ini => "rtps_disc.ini", w_ini => "rtps_disc.ini",
    },
    "MutableUnionNoMatchName" => {
      reader_type => "MutableUnion", writer_type => "ModifiedNameMutableUnion",
      expect_inconsistent_topic => 1, topic => "MutableUnionT_NoMatchName", key_val => 13,
      r_ini => "rtps_disc.ini", w_ini => "rtps_disc.ini",
    },
    "Tryconstruct" => {
      reader_type => "Trim20Struct", writer_type => "Trim64Struct",
      expect_inconsistent_topic => 0, topic => "TryconstructT", key_val => 0,
      r_ini => "rtps_disc.ini", w_ini => "rtps_disc.ini",
    },
    "Dependency" => {
      reader_type => "AppendableStruct", writer_type => "AppendableStructWithDependency",
      expect_inconsistent_topic => 0, topic => "DependencyT", key_val => 14,
      r_ini => "rtps_disc.ini", w_ini => "rtps_disc.ini",
    },

    "Match_no_xtypes_nn" => {
      reader_type => "AppendableStructNoXTypes", writer_type => "AppendableStructNoXTypes",
      expect_inconsistent_topic => 0, topic => "AppendableStructT_no_xtypes_nn",
      key_val => 4, r_ini => "rtps_disc_no_xtypes.ini", w_ini => "rtps_disc_no_xtypes.ini",
    },
    "Match_no_xtypes_yn" => {
      reader_type => "AppendableStructNoXTypes", writer_type => "AppendableStructNoXTypes",
      expect_inconsistent_topic => 0, topic => "AppendableStructT_no_xtypes_yn",
      key_val => 4, r_ini => "rtps_disc_no_xtypes.ini", w_ini => "rtps_disc.ini",
    },
    "Match_no_xtypes_ny" => {
      reader_type => "AppendableStructNoXTypes", writer_type => "AppendableStructNoXTypes",
      expect_inconsistent_topic => 0, topic => "AppendableStructT_no_xtypes_ny",
      key_val => 4, r_ini => "rtps_disc.ini", w_ini => "rtps_disc_no_xtypes.ini",
    },

    # Type consistency enforcement Qos policy tests
    "IgnoreMemberNames_MutableStructMatch" => {
      reader_type => "MutableStruct", writer_type => "ModifiedNameMutableStruct",
      expect_inconsistent_topic => 0, topic => "MutableStruct_IMN", key_val => 1,
      r_ini => "rtps_disc.ini", w_ini => "rtps_disc.ini",
      reader_args => "--ignore_member_names",
    },
    "IgnoreMemberNames_MutableUnionMatch" => {
      reader_type => "MutableUnion", writer_type => "ModifiedNameMutableUnion",
      expect_inconsistent_topic => 0, topic => "MutableUnion_IMN", key_val => 1,
      r_ini => "rtps_disc.ini", w_ini => "rtps_disc.ini",
      reader_args => "--ignore_member_names",
    },
    "DisallowTypeCoercion_FinalStructMatch" => {
      reader_type => "FinalStructSub", writer_type => "FinalStructPub",
      expect_inconsistent_topic => 0, topic => "FinalStructMatch_DTC", key_val => 1,
      r_ini => "rtps_disc.ini", w_ini => "rtps_disc.ini",
      reader_args => "--disallow_type_coercion",
    },
    "DisallowTypeCoercion_FinalStructNoMatch" => {
      reader_type => "FinalStructSub", writer_type => "ModifiedFinalStruct",
      expect_inconsistent_topic => 1, topic => "FinalStructNoMatch_DTC", key_val => 1,
      r_ini => "rtps_disc.ini", w_ini => "rtps_disc.ini",
      reader_args => "--disallow_type_coercion",
    },
    "DisallowTypeCoercion_AppendableStructNoMatch1" => {
      reader_type => "AppendableStruct", writer_type => "AdditionalPostfixFieldStruct",
      expect_inconsistent_topic => 1, topic => "AppendableStructNoMatch1_DTC", key_val => 1,
      r_ini => "rtps_disc.ini", w_ini => "rtps_disc.ini",
      reader_args => "--disallow_type_coercion",
    },
    "DisallowTypeCoercion_AppendableStructNoMatch2" => {
      reader_type => "AppendableStruct", writer_type => "AdditionalPrefixFieldStruct",
      expect_inconsistent_topic => 1, topic => "AppendableStructNoMatch2_DTC", key_val => 1,
      r_ini => "rtps_disc.ini", w_ini => "rtps_disc.ini",
      reader_args => "--disallow_type_coercion",
    },
    "DisallowTypeCoercion_MutableStructMatch" => {
      reader_type => "MutableStruct", writer_type => "ModifiedMutableStruct",
      expect_inconsistent_topic => 0, topic => "MutableStructMatch_DTC", key_val => 1,
      r_ini => "rtps_disc.ini", w_ini => "rtps_disc.ini",
      reader_args => "--disallow_type_coercion",
    },
    "DisallowTypeCoercion_MutableStructNoMatchId" => {
      reader_type => "MutableStruct", writer_type => "ModifiedIdMutableStruct",
      expect_inconsistent_topic => 1, topic => "MutableStructNoMatchId_DTC", key_val => 1,
      r_ini => "rtps_disc.ini", w_ini => "rtps_disc.ini",
      reader_args => "--disallow_type_coercion",
    },
    "DisallowTypeCoercion_MutableStructNoMatchType" => {
      reader_type => "MutableStruct", writer_type => "ModifiedTypeMutableStruct",
      expect_inconsistent_topic => 1, topic => "MutableStructNoMatchType_DTC", key_val => 1,
      r_ini => "rtps_disc.ini", w_ini => "rtps_disc.ini",
      reader_args => "--disallow_type_coercion",
    },
    "DisallowTypeCoercion_MutableUnionNoMatch" => {
      reader_type => "MutableUnion", writer_type => "ModifiedMutableUnion",
      expect_inconsistent_topic => 1, topic => "MutableUnionNoMatch_DTC", key_val => 1,
      r_ini => "rtps_disc.ini", w_ini => "rtps_disc.ini",
      reader_args => "--disallow_type_coercion",
    },
    "DisallowTypeCoercion_MutableUnionNoMatchType" => {
      reader_type => "MutableUnion", writer_type => "ModifiedTypeMutableUnion",
      expect_inconsistent_topic => 1, topic => "MutableUnionNoMatchType_DTC", key_val => 1,
      r_ini => "rtps_disc.ini", w_ini => "rtps_disc.ini",
      reader_args => "--disallow_type_coercion",
    },
    "ForceTypeValidation_NoXTypesNoMatch_nn" => {
      reader_type => "AppendableStructNoXTypes", writer_type => "AppendableStructNoXTypes",
      expect_inconsistent_topic => 1, topic => "NoXTypes_FTV_nn", key_val => 1,
      r_ini => "rtps_disc_no_xtypes.ini", w_ini => "rtps_disc_no_xtypes.ini",
      reader_args => "--force_type_validation",
    },
    "ForceTypeValidation_NoXTypesNoMatch_yn" => {
      reader_type => "AppendableStructNoXTypes", writer_type => "AppendableStructNoXTypes",
      expect_inconsistent_topic => 1, topic => "NoXTypes_FTV_yn", key_val => 1,
      r_ini => "rtps_disc_no_xtypes.ini", w_ini => "rtps_disc.ini",
      reader_args => "--force_type_validation",
    },
    "ForceTypeValidation_NoXTypesNoMatch_ny" => {
      reader_type => "AppendableStructNoXTypes", writer_type => "AppendableStructNoXTypes",
      expect_inconsistent_topic => 1, topic => "NoXTypes_FTV_ny", key_val => 1,
      r_ini => "rtps_disc.ini", w_ini => "rtps_disc_no_xtypes.ini",
      reader_args => "--force_type_validation",
    },
    "NoXTypesMatchTypeNames" => {
      reader_type => "AppendableStructNoXTypes", writer_type => "AppendableStructNoXTypes",
      r_reg_type => "CommonTypeName", w_reg_type => "CommonTypeName",
      expect_inconsistent_topic => 0, topic => "NoXTypesMatchTypeNames_nn", key_val => 1,
      r_ini => "rtps_disc_no_xtypes.ini", w_ini => "rtps_disc_no_xtypes.ini",
    },
    "NoXTypesNoMatchTypeNames" => {
      reader_type => "AppendableStructNoXTypes", writer_type => "AppendableStructNoXTypes",
      r_reg_type => "ReaderTypeName", w_reg_type => "WriterTypeName",
      expect_inconsistent_topic => 1, topic => "NoXTypesNoMatchTypeNames_nn", key_val => 1,
      r_ini => "rtps_disc_no_xtypes.ini", w_ini => "rtps_disc_no_xtypes.ini",
    },
  );
}

my $status = 0;

sub run_test {
  my @test_args;
  push(@test_args, @common_args);

  my $v = $_[0];
  my $test_name_param = $_[1];

  if ($v->{topic}) {
    push(@test_args, "--topic $v->{topic}");
  }

  if ($v->{expect_inconsistent_topic} > 0) {
    push(@test_args, "--expect_inconsistent_topic");
  }
  # This is used to check that a writer with no representation QoS and a reader who specifies XCDR2
  # will not match on a non rtps_udp transport. The writer should default to UnalignedCDR, and this should
  # be caught in the QoS consistency checks. Otherwise, the --tcp version of this test will ensure that a
  # writer with XCDR2 in a non rtps_udp transport is properly associating with a reader who does not specify
  # representation QoS and is notifying the reader that cdr_encapsulation() is enabled.
  if ($v->{expect_incompatible_qos} > 0) {
    push(@test_args, "--expect_incompatible_qos");
  }

  if ($v->{key_val} >= 0) {
    push(@test_args, "--key_val $v->{key_val}");
  }

  my @reader_args = ("-DCPSConfigFile $v->{r_ini} -ORBLogFile subscriber_$test_name_param.log --type $v->{reader_type}");
  if ($v->{reader_args}) {
    push(@reader_args, "$v->{reader_args}");
  }
  if ($v->{r_reg_type}) {
    push(@reader_args, "--reg_type $v->{r_reg_type}");
  }

  push(@reader_args, @test_args);
  $test->process("reader_$test_name_param", './subscriber', join(' ', @reader_args));
  $test->start_process("reader_$test_name_param");

  my @writer_args = ("-DCPSConfigFile $v->{w_ini} -ORBLogFile publisher_$test_name_param.log --type $v->{writer_type}");
  if ($v->{w_reg_type}) {
    push(@writer_args, "--reg_type $v->{w_reg_type}");
  }

  push(@writer_args, @test_args);
  $test->process("writer_$test_name_param", './publisher', join(' ', @writer_args));
  $test->start_process("writer_$test_name_param");

  $status |= $test->wait_kill("reader_$test_name_param", 30);
  $status |= $test->wait_kill("writer_$test_name_param", 30);
}

if ($test_name eq '') {
  while (my ($k, $v) = each %params) {
    run_test($v, $k);
  }
} else {
  run_test($params{$test_name}, $test_name);
}

if ($status) {
  print STDERR "ERROR: test failed\n";
}

exit $test->finish(60);
