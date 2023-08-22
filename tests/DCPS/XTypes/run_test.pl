eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
     & eval 'exec perl -S $0 $argv:q'
     if 0;

use strict;

use lib "$ENV{ACE_ROOT}/bin";
use lib "$ENV{DDS_ROOT}/bin";
use PerlDDS::Run_Test;
use Getopt::Long;

my $secure = 0;
my $tcp = 0;
my $verbose = 0;
my $run_test_name = "";
my $dynamic_writers = 0;
my $dynamic_readers = 0;
GetOptions(
  "secure" => \$secure,
  "tcp" => \$tcp,
  "verbose" => \$verbose,
  "test|t=s" => \$run_test_name,
  "dynamic-writers" => \$dynamic_writers,
  "dynamic-readers" => \$dynamic_readers,
) or die("Invalid options");

my $governance_generated = 0;

my $default_ini;
my %all_test_params;
if ($secure) {
  $default_ini = 'rtps_disc_security.ini';
  %all_test_params = (
    "FinalStructMatch_s" => {
      reader_type => "FinalStructSub", writer_type => "FinalStructPub",
      topic => "PD_OL_OA_OM_OD",
    },
    "AppendableMatch_s" => {
      reader_type => "AppendableStruct", writer_type => "AdditionalPostfixFieldStruct",
      topic => "PD_OL_OA_OM_OD",
    },
    "AppendableNoMatch_s" => {
      reader_type => "AppendableStruct", writer_type => "AdditionalPrefixFieldStruct",
      expect_inconsistent_topic => 1, topic => "PD_OL_OA_OM_OD",
    },
    "Dependency_s" => {
      reader_type => "AppendableStruct", writer_type => "AppendableStructWithDependency",
      topic => "PD_OL_OA_OM_OD",
    },
  );

} elsif ($tcp) {
  $default_ini = 'tcp.ini';
  %all_test_params = (
    "Tcp_MutableUnionNoMatchQos" => {
      reader_type => "MutableUnion", writer_type => "ModifiedMutableUnion",
      topic => "MutableUnionT", expect_incompatible_qos => 1
    },
    "Tcp_AppendableMatch" => {
      reader_type => "AppendableStruct", writer_type => "AdditionalPostfixFieldStruct",
      topic => "AppendableStructT",
    },
    "Tcp_AppendableNoMatch" => {
      reader_type => "AppendableStruct", writer_type => "AdditionalPrefixFieldStruct",
      expect_inconsistent_topic => 1, topic => "AppendableStructT_NoMatch",
    },
    "Tcp_MutableStruct" => {
      reader_type => "MutableStruct", writer_type => "ModifiedMutableStruct",
      topic => "MutableStructT",
    },
    "Tcp_MutableBaseStruct" => {
      reader_type => "MutableStruct", writer_type => "MutableBaseStruct",
      topic => "MutableBaseStructT",
    },
    "Tcp_MutableStructNoMatchName" => {
      reader_type => "MutableStruct", writer_type => "ModifiedNameMutableStruct",
      expect_inconsistent_topic => 1, topic => "MutableStructT_NoMatchName",
    },
    "Tcp_MutableUnion" => {
      reader_type => "MutableUnion", writer_type => "ModifiedMutableUnion",
      topic => "MutableUnionT",
    },
    "Tcp_MutableUnionNoMatchDisc" => {
      reader_type => "MutableUnion", writer_type => "ModifiedDiscMutableUnion",
      expect_inconsistent_topic => 1, topic => "MutableUnionT_NoMatchDisc",
    },
  );

} else {
  $default_ini = 'rtps_disc.ini';
  %all_test_params = (
    "PlainCdr" => {
      reader_type => "PlainCdrStruct", writer_type => "PlainCdrStruct",
      topic => 'PlainCdr',
    },
    "FinalStructMatch" => {
      reader_type => "FinalStructSub", writer_type => "FinalStructPub",
      topic => "FinalStructT",
    },
    "FinalStructNoMatch" => {
      reader_type => "FinalStructSub", writer_type => "ModifiedFinalStruct",
      expect_inconsistent_topic => 1, topic => "FinalStructT_F",
    },
    "AppendableMatch" => {
      reader_type => "AppendableStruct", writer_type => "AdditionalPostfixFieldStruct",
      topic => "AppendableStructT",
    },
    "ExtendedAppendableMatch" => {
      reader_type => "ExtendedAppendableStruct", writer_type => "BaseAppendableStruct",
      topic => "AppendableStructT",
      skip_read_if_dynamic => 1, # TODO: Remove once this is supported
    },
    "AppendableNoMatch" => {
      reader_type => "AppendableStruct", writer_type => "AdditionalPrefixFieldStruct",
      expect_inconsistent_topic => 1, topic => "AppendableStructT_NoMatch",
    },
    "MutableStruct" => {
      reader_type => "MutableStruct", writer_type => "ModifiedMutableStruct",
      topic => "MutableStructT",
    },
    "ExtendedMutableStructMatch" => {
      reader_type => "MutableStruct", writer_type => "MutableBaseStruct",
      topic => "MutableBaseStructT",
      skip_read_if_dynamic => 1, # TODO: Remove once this is supported
    },
    "MutableStructNoMatchId" => {
      reader_type => "MutableStruct", writer_type => "ModifiedIdMutableStruct",
      expect_inconsistent_topic => 1, topic => "MutableStructT_NoMatchId",
    },
    "MutableStructNoMatchType" => {
      reader_type => "MutableStruct", writer_type => "ModifiedTypeMutableStruct",
      expect_inconsistent_topic => 1, topic => "MutableStructT_NoMatchType",
    },
    "MutableStructNoMatchName" => {
      reader_type => "MutableStruct", writer_type => "ModifiedNameMutableStruct",
      expect_inconsistent_topic => 1, topic => "MutableStructT_NoMatchName",
    },
    "MutableUnion" => {
      reader_type => "MutableUnion", writer_type => "ModifiedMutableUnion",
      topic => "MutableUnionT",
    },
    "MutableUnionNoMatchDisc" => {
      reader_type => "MutableUnion", writer_type => "ModifiedDiscMutableUnion",
      expect_inconsistent_topic => 1, topic => "MutableUnionT_NoMatchDisc",
    },
    "MutableUnionNoMatchType" => {
      reader_type => "MutableUnion", writer_type => "ModifiedTypeMutableUnion",
      expect_inconsistent_topic => 1, topic => "MutableUnionT_NoMatchType",
    },
    "MutableUnionNoMatchName" => {
      reader_type => "MutableUnion", writer_type => "ModifiedNameMutableUnion",
      expect_inconsistent_topic => 1, topic => "MutableUnionT_NoMatchName",
    },
    "Tryconstruct" => {
      reader_type => "Trim20Struct", writer_type => "Trim64Struct",
      topic => "TryconstructT",
      skip_read_if_dynamic => 1, # TODO: Remove once this is supported
    },
    "Dependency" => {
      reader_type => "AppendableStruct", writer_type => "AppendableStructWithDependency",
      topic => "DependencyT",
    },

    "Match_no_xtypes_nn" => {
      reader_type => "AppendableStructNoXTypes", writer_type => "AppendableStructNoXTypes",
      topic => "AppendableStructT_no_xtypes_nn", ini => "rtps_disc_no_xtypes.ini",
    },
    "Match_no_xtypes_yn" => {
      reader_type => "AppendableStructNoXTypes", writer_type => "AppendableStructNoXTypes",
      topic => "AppendableStructT_no_xtypes_yn", r_ini => "rtps_disc_no_xtypes.ini",
    },
    "Match_no_xtypes_ny" => {
      reader_type => "AppendableStructNoXTypes", writer_type => "AppendableStructNoXTypes",
      topic => "AppendableStructT_no_xtypes_ny", w_ini => "rtps_disc_no_xtypes.ini",
    },

    # Type consistency enforcement Qos policy tests
    "IgnoreMemberNames_MutableStructMatch" => {
      reader_type => "MutableStruct", writer_type => "ModifiedNameMutableStruct",
      topic => "MutableStruct_IMN", reader_args => "--ignore-member-names",
    },
    "IgnoreMemberNames_MutableUnionMatch" => {
      reader_type => "MutableUnion", writer_type => "ModifiedNameMutableUnion",
      topic => "MutableUnion_IMN", reader_args => "--ignore-member-names",
    },
    "DisallowTypeCoercion_FinalStructMatch" => {
      reader_type => "FinalStructSub", writer_type => "FinalStructPub",
      topic => "FinalStructMatch_DTC", reader_args => "--disallow-type-coercion",
    },
    "DisallowTypeCoercion_FinalStructNoMatch" => {
      reader_type => "FinalStructSub", writer_type => "ModifiedFinalStruct",
      expect_inconsistent_topic => 1, topic => "FinalStructNoMatch_DTC",
      reader_args => "--disallow-type-coercion",
    },
    "DisallowTypeCoercion_AppendableStructNoMatch1" => {
      reader_type => "AppendableStruct", writer_type => "AdditionalPostfixFieldStruct",
      expect_inconsistent_topic => 1, topic => "AppendableStructNoMatch1_DTC",
      reader_args => "--disallow-type-coercion",
    },
    "DisallowTypeCoercion_AppendableStructNoMatch2" => {
      reader_type => "AppendableStruct", writer_type => "AdditionalPrefixFieldStruct",
      expect_inconsistent_topic => 1, topic => "AppendableStructNoMatch2_DTC",
      reader_args => "--disallow-type-coercion",
    },
    "DisallowTypeCoercion_MutableStructNoMatch" => {
      reader_type => "MutableStruct", writer_type => "ModifiedMutableStruct",
      expect_inconsistent_topic => 1, topic => "MutableStructMatch_DTC",
      reader_args => "--disallow-type-coercion",
    },
    "DisallowTypeCoercion_MutableStructNoMatchId" => {
      reader_type => "MutableStruct", writer_type => "ModifiedIdMutableStruct",
      expect_inconsistent_topic => 1, topic => "MutableStructNoMatchId_DTC",
      reader_args => "--disallow-type-coercion",
    },
    "DisallowTypeCoercion_MutableStructNoMatchType" => {
      reader_type => "MutableStruct", writer_type => "ModifiedTypeMutableStruct",
      expect_inconsistent_topic => 1, topic => "MutableStructNoMatchType_DTC",
      reader_args => "--disallow-type-coercion",
    },
    "DisallowTypeCoercion_MutableUnionNoMatch" => {
      reader_type => "MutableUnion", writer_type => "ModifiedMutableUnion",
      expect_inconsistent_topic => 1, topic => "MutableUnionNoMatch_DTC",
      reader_args => "--disallow-type-coercion",
    },
    "DisallowTypeCoercion_MutableUnionNoMatchType" => {
      reader_type => "MutableUnion", writer_type => "ModifiedTypeMutableUnion",
      expect_inconsistent_topic => 1, topic => "MutableUnionNoMatchType_DTC",
      reader_args => "--disallow-type-coercion",
    },
    "ForceTypeValidation_NoXTypesNoMatch_nn" => {
      reader_type => "AppendableStructNoXTypes", writer_type => "AppendableStructNoXTypes",
      expect_inconsistent_topic => 1, topic => "NoXTypes_FTV_nn",
      ini => "rtps_disc_no_xtypes.ini", reader_args => "--force-type-validation",
    },
    "ForceTypeValidation_NoXTypesNoMatch_yn" => {
      reader_type => "AppendableStructNoXTypes", writer_type => "AppendableStructNoXTypes",
      expect_inconsistent_topic => 1, topic => "NoXTypes_FTV_yn",
      r_ini => "rtps_disc_no_xtypes.ini", reader_args => "--force-type-validation",
    },
    "ForceTypeValidation_NoXTypesNoMatch_ny" => {
      reader_type => "AppendableStructNoXTypes", writer_type => "AppendableStructNoXTypes",
      expect_inconsistent_topic => 1, topic => "NoXTypes_FTV_ny",
      w_ini => "rtps_disc_no_xtypes.ini", reader_args => "--force-type-validation",
    },
    "NoXTypesMatchTypeNames" => {
      reader_type => "AppendableStructNoXTypes", writer_type => "AppendableStructNoXTypes",
      r_reg_type => "CommonTypeName", w_reg_type => "CommonTypeName",
      topic => "NoXTypesMatchTypeNames_nn", ini => "rtps_disc_no_xtypes.ini",
    },
    "NoXTypesNoMatchTypeNames" => {
      reader_type => "AppendableStructNoXTypes", writer_type => "AppendableStructNoXTypes",
      r_reg_type => "ReaderTypeName", w_reg_type => "WriterTypeName",
      expect_inconsistent_topic => 1, topic => "NoXTypesNoMatchTypeNames_nn",
      ini => "rtps_disc_no_xtypes.ini",
    },
  );
}

my %valid_params = ( # 1 = Required, 0 = Optional
  topic => 1, reader_type => 1, writer_type => 1,
  ini => 0, skip_read_if_dynamic => 0,
  r_reg_type => 0, r_ini => 0, reader_args => 0,
  w_reg_type => 0, w_ini => 0, writer_args => 0,
  expect_inconsistent_topic => 0, expect_incompatible_qos => 0,
);
my $key_val = 100;
for my $test_name (keys(%all_test_params)) {
  my $params = $all_test_params{$test_name};

  my @missing_params = grep { $valid_params{$_} && !exists($params->{$_}) } keys(%valid_params);
  die("$test_name has missing required params: ",
    join(', ', @missing_params)) if (scalar(@missing_params));
  my @invalid_params = grep { !exists($valid_params{$_}) } keys(%{$params});
  die("$test_name has invalid params: ", join(', ', @invalid_params)) if (scalar(@invalid_params));

  if (!exists($params->{key_val})) {
    $params->{key_val} = $key_val;
    $key_val += 1;
  }

  my $ini = $params->{ini} // $default_ini;
  $params->{r_ini} = $ini if (!exists($params->{r_ini}));
  $params->{w_ini} = $ini if (!exists($params->{w_ini}));
}

my @failed;

sub run_test {
  my $name = shift;
  print("----- $name ", '-' x 50, "\n");
  if (!exists($all_test_params{$name})) {
    die("$name is not a valid test");
  }
  my $params = $all_test_params{$name};

  my $test = new PerlDDS::TestFramework();
  $test->enable_console_logging();
  if ($secure && !$governance_generated) {
    $test->generate_governance("AU_UA_ND_NL_NR", "governance.xml.p7s");
    $governance_generated = 1;
  }

  my @common_args = (
    '-ORBDebugLevel 1 -DCPSDebugLevel 6',
    "--key-val $params->{key_val}",
    "--topic $params->{topic}",
  );
  push(@common_args, "--verbose") if ($verbose);
  push(@common_args, "--expect-inconsistent-topic") if ($params->{expect_inconsistent_topic});
  # This is used to check that a writer with no representation QoS and a reader who specifies XCDR2
  # will not match on a non rtps_udp transport. The writer should default to UnalignedCDR, and this should
  # be caught in the QoS consistency checks. Otherwise, the --tcp version of this test will ensure that a
  # writer with XCDR2 in a non rtps_udp transport is properly associating with a reader who does not specify
  # representation QoS and is notifying the reader that cdr_encapsulation() is enabled.
  push(@common_args, "--expect-incompatible-qos") if ($params->{expect_incompatible_qos});

  my @reader_args = (
    @common_args,
    "-DCPSConfigFile $params->{r_ini}",
    "-ORBLogFile subscriber_$name.log",
    "--type $params->{reader_type}",
  );
  push(@reader_args, $params->{reader_args}) if ($params->{reader_args});
  push(@reader_args, "--reg-type $params->{r_reg_type}") if ($params->{r_reg_type});
  if ($dynamic_readers) {
    push(@reader_args, '--dynamic-ts');
    push(@reader_args, '--skip-read') if ($params->{skip_read_if_dynamic});
  }

  my $reader = "reader_$name";
  $test->process($reader, './subscriber', join(' ', @reader_args));
  $test->start_process($reader);

  my @writer_args = (
    @common_args,
    "-DCPSConfigFile $params->{w_ini}",
    "-ORBLogFile publisher_$name.log",
    "--type $params->{writer_type}",
  );
  push(@writer_args, "--reg-type $params->{w_reg_type}") if ($params->{w_reg_type});
  push(@writer_args, '--dynamic-ts') if ($dynamic_writers);

  my $writer = "writer_$name";
  $test->process($writer, './publisher', join(' ', @writer_args));
  $test->start_process($writer);

  $test->stop_process(30, $reader);
  $test->stop_process(30, $writer);
  if ($test->finish(60)) {
    print("$name failed\n");
    push(@failed, $name);
  }
}

if ($run_test_name) {
  run_test($run_test_name);
} else {
  for my $test_name (keys(%all_test_params)) {
    run_test($test_name);
  }
}

if (scalar(@failed)) {
  for my $name (@failed) {
    print("$name failed\n");
  }
  exit(1);
}

