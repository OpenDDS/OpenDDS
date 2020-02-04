eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# -*- perl -*-

my @original_ARGV = @ARGV;

use Env (DDS_ROOT);
use lib "$DDS_ROOT/bin";
use Env (ACE_ROOT);
use lib "$ACE_ROOT/bin";
use PerlDDS::Run_Test;
use strict;

use Time::Piece;
use Time::Seconds;
use Getopt::Long;

my $scenario;
my @gov_files;
my $pub_cfg_file = "sec_base.ini";
my $sub_cfg_file = "sec_base.ini";
my $pub_cert_file = "../certs/identity/test_participant_01_cert.pem";
my $sub_cert_file = "../certs/identity/test_participant_02_cert.pem";
my $pub_key_file = "../certs/identity/test_participant_01_private_key.pem";
my $sub_key_file = "../certs/identity/test_participant_02_private_key.pem";
my @pub_perm_files;
my @sub_perm_files;
my @topic_names;
my $pub_expect = "0";
my $sub_expect = "0";
my $pub_timeout = "10";
my $sub_timeout = "10";
my $pub_extra_space = "0";

GetOptions(
  'scenario=s' => \$scenario,
  'pub_cfg=s' => \$pub_cfg_file,
  'sub_cfg=s' => \$sub_cfg_file,
  'pub_cert=s' => \$pub_cert_file,
  'sub_cert=s' => \$sub_cert_file,
  'pub_key=s' => \$pub_key_file,
  'sub_key=s' => \$sub_key_file,
  'gov=s' => \@gov_files,
  'pub_perm=s' => \@pub_perm_files,
  'sub_perm=s' => \@sub_perm_files,
  'topic=s' => \@topic_names,
  'pub_expect=i' => \$pub_expect,
  'sub_expect=i' => \$sub_expect,
  'pub_timeout=i' => \$pub_timeout,
  'sub_timeout=i' => \$sub_timeout,
  'pub_extra_space=i' => \$pub_extra_space);

# Handle scenarios first, since they are a special case
if (!($scenario eq "")) {
  if ($scenario eq "SC0_sec_off") { #SC0 (open domain interop w/ unsecure) : unsecure -> unsecure
    $pub_cfg_file = "unsec_base.ini";
    $sub_cfg_file = "unsec_base.ini";
    @gov_files = ("governance/governance_AU_UA_ND_NL_NR_signed.p7s");
    @pub_perm_files = ("permissions/permissions_test_participant_01_allowall_signed.p7s");
    @sub_perm_files = ("permissions/permissions_test_participant_02_allowall_signed.p7s");
    @topic_names = ("OD_OL_OA_OM_OD");
  } elsif ($scenario eq "SC0_sec_sub") { #SC0 (open domain interop w/ unsecure) : unsecure -> secure
    $sub_cfg_file = "unsec_base.ini";
    @gov_files = ("governance/governance_AU_UA_ND_NL_NR_signed.p7s");
    @pub_perm_files = ("permissions/permissions_test_participant_01_allowall_signed.p7s");
    @sub_perm_files = ("permissions/permissions_test_participant_02_allowall_signed.p7s");
    @topic_names = ("OD_OL_OA_OM_OD");
    $pub_timeout = $sub_timeout = 20;
  } elsif ($scenario eq "SC0_sec_pub") { #SC0 (open domain interop w/ unsecure) : secure -> unsecure
    $pub_cfg_file = "unsec_base.ini";
    @gov_files = ("governance/governance_AU_UA_ND_NL_NR_signed.p7s");
    @pub_perm_files = ("permissions/permissions_test_participant_01_allowall_signed.p7s");
    @sub_perm_files = ("permissions/permissions_test_participant_02_allowall_signed.p7s");
    @topic_names = ("OD_OL_OA_OM_OD");
    $pub_timeout = $sub_timeout = 20;
  } elsif ($scenario eq "SC0_sec_on") { #SC0 (open domain interop w/ unsecure) : secure -> secure
    @gov_files = ("governance/governance_AU_UA_ND_NL_NR_signed.p7s");
    @pub_perm_files = ("permissions/permissions_test_participant_01_allowall_signed.p7s");
    @sub_perm_files = ("permissions/permissions_test_participant_02_allowall_signed.p7s");
    @topic_names = ("OD_OL_OA_OM_OD");
  } elsif ($scenario eq "SC0_sec_on_ec_pub") { #SC0 (open domain interop w/ unsecure) : secure -> secure (eliptical curve cert for pub)
    $pub_cert_file = "../certs/identity/test_participant_03_cert.pem";
    $pub_key_file = "../certs/identity/test_participant_03_private_key.pem";
    @gov_files = ("governance/governance_AU_UA_ND_NL_NR_signed.p7s");
    @pub_perm_files = ("permissions/permissions_test_participant_03_allowall_signed.p7s");
    @sub_perm_files = ("permissions/permissions_test_participant_02_allowall_signed.p7s");
    @topic_names = ("OD_OL_OA_OM_OD");
  } elsif ($scenario eq "SC0_sec_on_ec_sub") { #SC0 (open domain interop w/ unsecure) : secure -> secure (eliptical curve certs for sub)
    $sub_cert_file = "../certs/identity/test_participant_04_cert.pem";
    $sub_key_file = "../certs/identity/test_participant_04_private_key.pem";
    @gov_files = ("governance/governance_AU_UA_ND_NL_NR_signed.p7s");
    @pub_perm_files = ("permissions/permissions_test_participant_01_allowall_signed.p7s");
    @sub_perm_files = ("permissions/permissions_test_participant_04_allowall_signed.p7s");
    @topic_names = ("OD_OL_OA_OM_OD");
  } elsif ($scenario eq "SC0_sec_on_ec_both") { #SC0 (open domain interop w/ unsecure) : secure -> secure (eliptical curve certs for both)
    $pub_cert_file = "../certs/identity/test_participant_03_cert.pem";
    $sub_cert_file = "../certs/identity/test_participant_04_cert.pem";
    $pub_key_file = "../certs/identity/test_participant_03_private_key.pem";
    $sub_key_file = "../certs/identity/test_participant_04_private_key.pem";
    @gov_files = ("governance/governance_AU_UA_ND_NL_NR_signed.p7s");
    @pub_perm_files = ("permissions/permissions_test_participant_03_allowall_signed.p7s");
    @sub_perm_files = ("permissions/permissions_test_participant_04_allowall_signed.p7s");
    @topic_names = ("OD_OL_OA_OM_OD");
  } elsif ($scenario eq "SC1_sec_off_success") { #SC1 (join controlled domain) : unsecure participants won't check governance
    $pub_cfg_file = "unsec_base.ini";
    $sub_cfg_file = "unsec_base.ini";
    @gov_files = ("governance/governance_PU_PA_ED_EL_NR_signed.p7s");
    @pub_perm_files = ("permissions/permissions_test_participant_01_readwrite_signed.p7s");
    @sub_perm_files = ("permissions/permissions_test_participant_02_readwrite_signed.p7s");
    @topic_names = ("OD_OL_OA_OM_OD");
  } elsif ($scenario eq "SC1_sec_sub_failure") { #SC1 (join controlled domain) : unsecure participants won't authenticate, secure participant ignores unauthenticated
    $pub_cfg_file = "unsec_base.ini";
    @gov_files = ("governance/governance_PU_PA_ED_EL_NR_signed.p7s");
    @pub_perm_files = ("permissions/permissions_test_participant_01_readwrite_signed.p7s");
    @sub_perm_files = ("permissions/permissions_test_participant_02_readwrite_signed.p7s");
    @topic_names = ("OD_OL_OA_OM_OD");
    $pub_expect = "~16";
    $sub_expect = "~27";
  } elsif ($scenario eq "SC1_sec_pub_failure") { #SC1 (join controlled domain) : unsecure participants won't authenticate, secure participant ignores unauthenticated
    $sub_cfg_file = "unsec_base.ini";
    @gov_files = ("governance/governance_PU_PA_ED_EL_NR_signed.p7s");
    @pub_perm_files = ("permissions/permissions_test_participant_01_readwrite_signed.p7s");
    @sub_perm_files = ("permissions/permissions_test_participant_02_readwrite_signed.p7s");
    @topic_names = ("OD_OL_OA_OM_OD");
    $pub_expect = "~16";
    $sub_expect = "~27";
  } elsif ($scenario eq "SC1_sec_on_bad_cert_failure") { #SC1 (join controlled domain) : secure participants with wrong credentials fail to authenticate
    $pub_key_file = "../certs/identity/test_participant_02_private_key.pem"; # This won't match cert (01)
    $sub_key_file = "../certs/identity/test_participant_01_private_key.pem"; # This won't match cert (02)
    @gov_files = ("governance/governance_PU_PA_ED_EL_NR_signed.p7s");
    @pub_perm_files = ("permissions/permissions_test_participant_01_readwrite_signed.p7s");
    @sub_perm_files = ("permissions/permissions_test_participant_02_readwrite_signed.p7s");
    @topic_names = ("OD_OL_OA_OM_OD");
    $pub_expect = "~16";
    $sub_expect = "~27";
  } elsif ($scenario eq "SC1_sec_on_bad_perm_1_failure") { #SC1 (join controlled domain) : secure participants with wrong permissions fail to validate
    @gov_files = ("governance/governance_PU_PA_ED_EL_NR_signed.p7s");
    @pub_perm_files = ("permissions/permissions_test_participant_01_join_wrong_signed.p7s"); # permissions domain doesn't match governance
    @sub_perm_files = ("permissions/permissions_test_participant_02_join_wrong_signed.p7s"); # permissions domain doesn't match governance
    @topic_names = ("OD_OL_OA_OM_OD");
    $pub_expect = "~16";
    $sub_expect = "~27";
  } elsif ($scenario eq "SC1_sec_on_bad_perm_2_failure") { #SC1 (join controlled domain) : secure participants with insufficient permissions fail to pass access control checks
    @gov_files = ("governance/governance_PU_PA_ED_EL_NR_signed.p7s");
    @pub_perm_files = ("permissions/permissions_test_participant_01_join_signed.p7s"); # doesn't have permission to write
    @sub_perm_files = ("permissions/permissions_test_participant_02_join_signed.p7s"); # doesn't have permission to read
    @topic_names = ("OD_OL_OA_OM_OD");
    $pub_expect = "~16";
    $sub_expect = "~27";
  } elsif ($scenario eq "SC1_sec_on_success") { #SC1 (join controlled domain) : valid participants join and send
    @gov_files = ("governance/governance_PU_PA_ED_EL_NR_signed.p7s");
    @pub_perm_files = ("permissions/permissions_test_participant_01_readwrite_signed.p7s");
    @sub_perm_files = ("permissions/permissions_test_participant_02_readwrite_signed.p7s");
    @topic_names = ("OD_OL_OA_OM_OD");
  } elsif ($scenario eq "SC2") {
    @gov_files = ("governance/governance_PU_PA_ED_EL_NR_signed.p7s");
    @pub_perm_files = ("permissions/permissions_test_participant_01_readwrite_signed.p7s");
    @sub_perm_files = ("permissions/permissions_test_participant_02_readwrite_signed.p7s");
    @topic_names = ("OD_OL_OA_EM_OD");
  } elsif ($scenario eq "SC3") {
    @gov_files = ("governance/governance_PU_PA_ED_EL_NR_signed.p7s");
    @pub_perm_files = ("permissions/permissions_test_participant_01_readwrite_signed.p7s");
    @sub_perm_files = ("permissions/permissions_test_participant_02_readwrite_signed.p7s");
    @topic_names = ("OD_OL_OA_OM_ED");
  } elsif ($scenario eq "SC4") {
    @gov_files = ("governance/governance_PU_PA_ED_EL_NR_signed.p7s");
    @pub_perm_files = ("permissions/permissions_test_participant_01_readwrite_signed.p7s");
    @sub_perm_files = ("permissions/permissions_test_participant_02_readwrite_signed.p7s");
    @topic_names = ("OD_OL_OA_SM_OD");
  } elsif ($scenario eq "SC5") {
    @gov_files = ("governance/governance_PU_PA_ED_EL_NR_signed.p7s");
    @pub_perm_files = ("permissions/permissions_test_participant_01_readwrite_signed.p7s");
    @sub_perm_files = ("permissions/permissions_test_participant_02_readwrite_signed.p7s");
    @topic_names = ("PD_OL_OA_OM_OD");
  } elsif ($scenario eq "multiple_grants") { #EC certs, read/write access controls and multiple grants in a single (huge) permissions file
    $pub_cert_file = "../certs/identity/test_participant_03_cert.pem";
    $sub_cert_file = "../certs/identity/test_participant_04_cert.pem";
    $pub_key_file = "../certs/identity/test_participant_03_private_key.pem";
    $sub_key_file = "../certs/identity/test_participant_04_private_key.pem";
    @gov_files = ("governance/governance_PU_PA_ED_EL_NR_signed.p7s");
    @pub_perm_files = ("permissions/permissions_multi_p_01_02_03_04_readwrite_signed.p7s");
    @sub_perm_files = ("permissions/permissions_multi_p_01_02_03_04_readwrite_signed.p7s");
    @topic_names = ("PD_OL_RWA_EM_ED");
  } elsif ($scenario eq "TEST_8_8_5_SUCCESS") {
    @gov_files = ("governance/governance_PU_PA_ND_NL_NR_signed.p7s");
    @pub_perm_files = ("permissions/permissions_test_participant_01_write_signed.p7s");
    @sub_perm_files = ("permissions/permissions_test_participant_02_read_signed.p7s");
    @topic_names = ("OD_OL_RWA_OM_OD");
  } elsif ($scenario eq "TEST_8_8_5_FAILURE") {
    @gov_files = ("governance/governance_PU_PA_ND_NL_NR_signed.p7s");
    @pub_perm_files = ("permissions/permissions_test_participant_01_read_signed.p7s");
    @sub_perm_files = ("permissions/permissions_test_participant_02_write_signed.p7s");
    @topic_names = ("OD_OL_RWA_OM_OD");
    $pub_expect = "~15";
    $sub_expect = "~25";
  } elsif ($scenario eq "FullMsgSign") {
    @gov_files = ("governance/governance_PU_PA_ED_NL_SR_signed.p7s");
    @pub_perm_files = ("permissions/permissions_test_participant_01_readwrite_signed.p7s");
    @sub_perm_files = ("permissions/permissions_test_participant_02_readwrite_signed.p7s");
    @topic_names = ("PD_OL_OA_OM_OD");
  } elsif ($scenario eq "FullMsgEncrypt") {
    @gov_files = ("governance/governance_PU_PA_ED_NL_ER_signed.p7s");
    @pub_perm_files = ("permissions/permissions_test_participant_01_readwrite_signed.p7s");
    @sub_perm_files = ("permissions/permissions_test_participant_02_readwrite_signed.p7s");
    @topic_names = ("PD_OL_OA_OM_OD");
  } elsif ($scenario eq "FullMsgSign_SubMsgSign") {
    @gov_files = ("governance/governance_PU_PA_ED_NL_SR_signed.p7s");
    @pub_perm_files = ("permissions/permissions_test_participant_01_readwrite_signed.p7s");
    @sub_perm_files = ("permissions/permissions_test_participant_02_readwrite_signed.p7s");
    @topic_names = ("PD_OL_OA_SM_OD");
  } elsif ($scenario eq "FullMsgSign_SubMsgEncrypt") {
    @gov_files = ("governance/governance_PU_PA_ED_NL_SR_signed.p7s");
    @pub_perm_files = ("permissions/permissions_test_participant_01_readwrite_signed.p7s");
    @sub_perm_files = ("permissions/permissions_test_participant_02_readwrite_signed.p7s");
    @topic_names = ("PD_OL_OA_EM_OD");
  } elsif ($scenario eq "FullMsgEncrypt_SubMsgSign") {
    @gov_files = ("governance/governance_PU_PA_ED_NL_ER_signed.p7s");
    @pub_perm_files = ("permissions/permissions_test_participant_01_readwrite_signed.p7s");
    @sub_perm_files = ("permissions/permissions_test_participant_02_readwrite_signed.p7s");
    @topic_names = ("PD_OL_OA_SM_OD");
  } elsif ($scenario eq "FullMsgEncrypt_SubMsgEncrypt") {
    @gov_files = ("governance/governance_PU_PA_ED_NL_ER_signed.p7s");
    @pub_perm_files = ("permissions/permissions_test_participant_01_readwrite_signed.p7s");
    @sub_perm_files = ("permissions/permissions_test_participant_02_readwrite_signed.p7s");
    @topic_names = ("PD_OL_OA_EM_OD");
  } elsif ($scenario eq "FullMsgSign_PayloadEncrypt") {
    @gov_files = ("governance/governance_PU_PA_ED_NL_SR_signed.p7s");
    @pub_perm_files = ("permissions/permissions_test_participant_01_readwrite_signed.p7s");
    @sub_perm_files = ("permissions/permissions_test_participant_02_readwrite_signed.p7s");
    @topic_names = ("PD_OL_OA_OM_ED");
  } elsif ($scenario eq "FullMsgEncrypt_PayloadEncrypt") {
    @gov_files = ("governance/governance_PU_PA_ED_NL_ER_signed.p7s");
    @pub_perm_files = ("permissions/permissions_test_participant_01_readwrite_signed.p7s");
    @sub_perm_files = ("permissions/permissions_test_participant_02_readwrite_signed.p7s");
    @topic_names = ("PD_OL_OA_OM_ED");
  } elsif ($scenario eq "FullMsgSign_SubMsgSign_PayloadEncrypt") {
    @gov_files = ("governance/governance_PU_PA_ED_NL_SR_signed.p7s");
    @pub_perm_files = ("permissions/permissions_test_participant_01_readwrite_signed.p7s");
    @sub_perm_files = ("permissions/permissions_test_participant_02_readwrite_signed.p7s");
    @topic_names = ("PD_OL_OA_SM_ED");
  } elsif ($scenario eq "FullMsgSign_SubMsgEncrypt_PayloadEncrypt") {
    @gov_files = ("governance/governance_PU_PA_ED_NL_SR_signed.p7s");
    @pub_perm_files = ("permissions/permissions_test_participant_01_readwrite_signed.p7s");
    @sub_perm_files = ("permissions/permissions_test_participant_02_readwrite_signed.p7s");
    @topic_names = ("PD_OL_OA_EM_ED");
  } elsif ($scenario eq "FullMsgEncrypt_SubMsgSign_PayloadEncrypt") {
    @gov_files = ("governance/governance_PU_PA_ED_NL_ER_signed.p7s");
    @pub_perm_files = ("permissions/permissions_test_participant_01_readwrite_signed.p7s");
    @sub_perm_files = ("permissions/permissions_test_participant_02_readwrite_signed.p7s");
    @topic_names = ("PD_OL_OA_SM_ED");
  } elsif ($scenario eq "FullMsgEncrypt_SubMsgEncrypt_PayloadEncrypt") {
    @gov_files = ("governance/governance_PU_PA_ED_NL_ER_signed.p7s");
    @pub_perm_files = ("permissions/permissions_test_participant_01_readwrite_signed.p7s");
    @sub_perm_files = ("permissions/permissions_test_participant_02_readwrite_signed.p7s");
    @topic_names = ("PD_OL_OA_EM_ED");
  } elsif ($scenario eq "NetProfiling_sec_off") {
    $pub_cfg_file = "unsec_base.ini";
    $sub_cfg_file = "unsec_base.ini";
    @gov_files = ("governance/governance_AU_UA_ND_NL_NR_signed.p7s");
    @pub_perm_files = ("permissions/permissions_test_participant_01_allowall_signed.p7s");
    @sub_perm_files = ("permissions/permissions_test_participant_02_allowall_signed.p7s");
    @topic_names = ("OD_OL_OA_OM_OD");
  } elsif ($scenario eq "NetProfiling_auth_acc") {
    @gov_files = ("governance/governance_PU_PA_ND_NL_NR_signed.p7s");
    @pub_perm_files = ("permissions/permissions_test_participant_01_allowall_signed.p7s");
    @sub_perm_files = ("permissions/permissions_test_participant_02_allowall_signed.p7s");
    @topic_names = ("OD_OL_RWA_OM_OD");
  } elsif ($scenario eq "NetProfiling_encrypt") {
    @gov_files = ("governance/governance_PU_PA_ED_EL_NR_signed.p7s");
    @pub_perm_files = ("permissions/permissions_test_participant_01_allowall_signed.p7s");
    @sub_perm_files = ("permissions/permissions_test_participant_02_allowall_signed.p7s");
    @topic_names = ("PD_OL_RWA_EM_ED");
  } elsif ($scenario eq "FullMsgSign_PayloadEncrypt_Frag") {
    @gov_files = ("governance/governance_PU_PA_ED_NL_SR_signed.p7s");
    @pub_perm_files = ("permissions/permissions_test_participant_01_readwrite_signed.p7s");
    @sub_perm_files = ("permissions/permissions_test_participant_02_readwrite_signed.p7s");
    @topic_names = ("PD_OL_OA_OM_ED");
    $pub_extra_space = "100000";
  } else {
    print "\nUnrecognized scenario '$scenario'. Skipping.\n";
    exit -1;
  }
} else { # Not using scenarios

  # Figure out what governance files to use
  if (scalar @gov_files == 0) {
    print "Using governance files from governance directory.\n";
    opendir(my $gov_dh, "governance");
    @gov_files = map {"governance/" . $_} (sort grep(/\.p7s$/,readdir($gov_dh)));
    closedir($gov_dh);

    # Filter out allow unauth + protected disc and prohibit unauth + unprotected discovery
    @gov_files = grep(!/_AU_PA/, @gov_files);
    @gov_files = grep(!/_PU_UA/, @gov_files);

    @gov_files = grep(!/_E.*_E./, @gov_files); # eliminate more than one encryption attribute

    @gov_files = grep(!/_S/, @gov_files); # eliminate signed stuff
    @gov_files = grep(!/_SO/, @gov_files); # eliminate origin authenticated signed stuff
    @gov_files = grep(!/_EO/, @gov_files); # eliminate origin authenticated encrypted stuff
  } else {
    print "Using governance " . (((scalar @gov_files) eq 1) ? "file" : "files") . ": '" . join ("', '", @gov_files) . "'.\n";
  }

  # Figure out which permissions files to use
  if (scalar @pub_perm_files == 0 || scalar @sub_perm_files == 0) {
    opendir(my $perm_dh, "permissions");
    my @perm_files = map {"permissions/" . $_} (sort grep(/\.p7s$/,readdir($perm_dh)));
    closedir($perm_dh);

    if (scalar @pub_perm_files == 0) {
      print "Using publisher permissions files from permissions directory.\n";
      @pub_perm_files = @perm_files;

      #Filter permissions files to 1st participant and full read/write
      @pub_perm_files = grep(/_test_participant_01/, @pub_perm_files);
      @pub_perm_files = grep(/_readwrite/, @pub_perm_files);

    } else {
      print "Using publisher permissions " . (((scalar @pub_perm_files) eq 1) ? "file" : "files") . ": '" . join ("', '", @pub_perm_files) . "'.\n";
    }

    if (scalar @sub_perm_files == 0) {
      print "Using subscriber permissions files from permissions directory.\n";
      @sub_perm_files = @perm_files;

      #Filter permissions files to 2nd participant and full read/write
      @sub_perm_files = grep(/_test_participant_02/, @sub_perm_files);
      @sub_perm_files = grep(/_readwrite/, @sub_perm_files);

    } else {
      print "Using subscriber permissions " . (((scalar @sub_perm_files) eq 1) ? "file" : "files") . ": '" . join ("', '", @sub_perm_files) . "'.\n";
    }
  } else {
    print "Using publisher permissions " . (((scalar @pub_perm_files) eq 1) ? "file" : "files") . ": '" . join ("', '", @pub_perm_files) . "'.\n";
    print "Using subscriber permissions " . (((scalar @sub_perm_files) eq 1) ? "file" : "files") . ": '" . join ("', '", @sub_perm_files) . "'.\n";
  }

  #Figure out which topics to use
  if (scalar @topic_names == 0) {
    print "Using topic names from topic_names.txt.\n";
    open my $topic_names_file, '<', "topic_names.txt";
    chomp(@topic_names = <$topic_names_file>);
    close $topic_names_file;

    #Filter out topics using signed values
    @topic_names = grep(!/_S/, @topic_names);

  } else {
    print "Using " . (((scalar @topic_names) eq 1) ? "topic" : "topics") . ": '" . join ("', '", @topic_names) . "'.\n";
  }
}

#open my $status_file, '>', "expected_status_results.txt";

my $total_test_count = (scalar @gov_files) * (scalar @pub_perm_files) * (scalar @sub_perm_files) * (scalar @topic_names);
my $current_test_num = 0;

my $test_start_time = localtime;

my $final_status = 0;

foreach my $gov_file (@gov_files) {
  foreach my $pub_perm_file (@pub_perm_files) {
    foreach my $sub_perm_file (@sub_perm_files) {
      foreach my $topic_name (@topic_names) {

        my $status = 0;

        my $test = new PerlDDS::TestFramework();

        $test->{dcps_debug_level} = 4;
        $test->{dcps_transport_debug_level} = 2;
        # will manually set -DCPSConfigFile
        $test->{add_transport_config} = 0;
        my $dbg_lvl = '-ORBDebugLevel 1';
        my $pub_opts = "$dbg_lvl";
        my $sub_opts = "$dbg_lvl";

        $pub_opts .= " -DCPSConfigFile $pub_cfg_file";
        $sub_opts .= " -DCPSConfigFile $sub_cfg_file";

        $pub_opts .= " -IdentityCA ../certs/identity/identity_ca_cert.pem";
        $sub_opts .= " -IdentityCA ../certs/identity/identity_ca_cert.pem";

        $pub_opts .= " -Identity $pub_cert_file";
        $sub_opts .= " -Identity $sub_cert_file";

        $pub_opts .= " -PrivateKey $pub_key_file";
        $sub_opts .= " -PrivateKey $sub_key_file";

        $pub_opts .= " -PermissionsCA ../certs/permissions/permissions_ca_cert.pem";
        $sub_opts .= " -PermissionsCA ../certs/permissions/permissions_ca_cert.pem";

        $pub_opts .= " -Permissions $pub_perm_file";
        $sub_opts .= " -Permissions $sub_perm_file";

        $pub_opts .= " -Governance $gov_file";
        $sub_opts .= " -Governance $gov_file";

        $pub_opts .= " -Topic $topic_name";
        $sub_opts .= " -Topic $topic_name";

        if (!($pub_expect eq "0")) {
          $pub_opts .= " -Expected $pub_expect";
        }

        if (!($sub_expect eq "0")) {
          $sub_opts .= " -Expected $sub_expect";
        }

        if (!($pub_timeout eq "0")) {
          $pub_opts .= " -Timeout $pub_timeout";
        }

        if (!($sub_timeout eq "0")) {
          $sub_opts .= " -Timeout $sub_timeout";
        }

        if (!($pub_extra_space eq "0")) {
          $pub_opts .= " -ExtraSpace $pub_extra_space";
        }

        #print "$gov_file $pub_perm_file $sub_perm_file\n";

        $test->process("publisher", "publisher", $pub_opts);
        $test->process("subscriber", "subscriber", $sub_opts);

        $test->start_process("subscriber");
        $test->start_process("publisher");

        # start killing processes in 10 seconds
        $test->{wait_after_first_proc} = 10;
        my $status = $test->finish(30);

        #if ($status != 0) {
          $current_test_num++;

          my $total_test_percent = (100.0 * $current_test_num) / $total_test_count;
          my $current_time = localtime;
          my $elapsed_time = $current_time - $test_start_time;
          my $estimate = $elapsed_time->seconds * ($total_test_count - $current_test_num) / ($current_test_num);

          if ($total_test_count != 1) {
            print "\ntest #$current_test_num of $total_test_count. Estimating $estimate seconds remaining.\n";
          }

          #print "$gov_file $pub_perm_file $sub_perm_file $topic_name $status\n\n-----------\n\n";
          #print $status_file "$gov_file $pub_perm_file $sub_perm_file $topic_name $status\n";

          #exit $status;
        #}

        if ($status != 0) {
          $final_status = -1;
        }
      }
    }
  }
}

exit $final_status;
