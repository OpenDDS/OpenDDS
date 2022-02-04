eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# -*- perl -*-

use Env (DDS_ROOT);
use lib "$DDS_ROOT/bin";
use Env (ACE_ROOT);
use lib "$ACE_ROOT/bin";
use PerlDDS::Run_Test;
use strict;

use Getopt::Long;

my $scenario;
my @gov_keys;
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
my @partition;
my @base_pub_args = ();
my @base_sub_args = ();

GetOptions(
  'pub_cfg=s' => \$pub_cfg_file,
  'sub_cfg=s' => \$sub_cfg_file,
  'pub_cert=s' => \$pub_cert_file,
  'sub_cert=s' => \$sub_cert_file,
  'pub_key=s' => \$pub_key_file,
  'sub_key=s' => \$sub_key_file,
  'gov=s' => \@gov_keys,
  'pub_perm=s' => \@pub_perm_files,
  'sub_perm=s' => \@sub_perm_files,
  'topic=s' => \@topic_names,
  'pub_expect=i' => \$pub_expect,
  'sub_expect=i' => \$sub_expect,
  'pub_timeout=i' => \$pub_timeout,
  'sub_timeout=i' => \$sub_timeout,
  'pub_extra_space=i' => \$pub_extra_space,
  'partition=s' => \@partition,
) or die("ERROR: Invalid options passed");

if (scalar(@ARGV) == 1) {
  $scenario = $ARGV[0];
}
elsif (scalar(@ARGV) > 1) {
  die("ERROR: Too many arguments passed");
}

# Handle scenarios first, since they are a special case
if ($scenario) {
  if ($scenario eq "SC0_sec_off") { #SC0 (open domain interop w/ unsecure) : unsecure -> unsecure
    $pub_cfg_file = "unsec_base.ini";
    $sub_cfg_file = "unsec_base.ini";
    @gov_keys = ("AU_UA_ND_NL_NR");
    @pub_perm_files = ("permissions/permissions_test_participant_01_allowall_signed.p7s");
    @sub_perm_files = ("permissions/permissions_test_participant_02_allowall_signed.p7s");
    @topic_names = ("OD_OL_OA_OM_OD");
  } elsif ($scenario eq "SC0_sec_sub") { #SC0 (open domain interop w/ unsecure) : unsecure -> secure
    $sub_cfg_file = "unsec_base.ini";
    @gov_keys = ("AU_UA_ND_NL_NR");
    @pub_perm_files = ("permissions/permissions_test_participant_01_allowall_signed.p7s");
    @sub_perm_files = ("permissions/permissions_test_participant_02_allowall_signed.p7s");
    @topic_names = ("OD_OL_OA_OM_OD");
    $pub_timeout = $sub_timeout = 20;
  } elsif ($scenario eq "SC0_sec_pub") { #SC0 (open domain interop w/ unsecure) : secure -> unsecure
    $pub_cfg_file = "unsec_base.ini";
    @gov_keys = ("AU_UA_ND_NL_NR");
    @pub_perm_files = ("permissions/permissions_test_participant_01_allowall_signed.p7s");
    @sub_perm_files = ("permissions/permissions_test_participant_02_allowall_signed.p7s");
    @topic_names = ("OD_OL_OA_OM_OD");
    $pub_timeout = $sub_timeout = 20;
  } elsif ($scenario eq "SC0_sec_on") { #SC0 (open domain interop w/ unsecure) : secure -> secure
    @gov_keys = ("AU_UA_ND_NL_NR");
    @pub_perm_files = ("permissions/permissions_test_participant_01_allowall_signed.p7s");
    @sub_perm_files = ("permissions/permissions_test_participant_02_allowall_signed.p7s");
    @topic_names = ("OD_OL_OA_OM_OD");
  } elsif ($scenario eq "SC0_sec_on_ec_pub") {
    # SC0 (open domain interop w/ unsecure) : secure -> secure (eliptical curve cert for pub)
    $pub_cert_file = "../certs/identity/test_participant_03_cert.pem";
    $pub_key_file = "../certs/identity/test_participant_03_private_key.pem";
    @gov_keys = ("AU_UA_ND_NL_NR");
    @pub_perm_files = ("permissions/permissions_test_participant_03_allowall_signed.p7s");
    @sub_perm_files = ("permissions/permissions_test_participant_02_allowall_signed.p7s");
    @topic_names = ("OD_OL_OA_OM_OD");
  } elsif ($scenario eq "SC0_sec_on_ec_sub") {
    # SC0 (open domain interop w/ unsecure) : secure -> secure (eliptical curve certs for sub)
    $sub_cert_file = "../certs/identity/test_participant_04_cert.pem";
    $sub_key_file = "../certs/identity/test_participant_04_private_key.pem";
    @gov_keys = ("AU_UA_ND_NL_NR");
    @pub_perm_files = ("permissions/permissions_test_participant_01_allowall_signed.p7s");
    @sub_perm_files = ("permissions/permissions_test_participant_04_allowall_signed.p7s");
    @topic_names = ("OD_OL_OA_OM_OD");
  } elsif ($scenario eq "SC0_sec_on_ec_both") {
    # SC0 (open domain interop w/ unsecure) : secure -> secure (eliptical curve certs for both)
    $pub_cert_file = "../certs/identity/test_participant_03_cert.pem";
    $sub_cert_file = "../certs/identity/test_participant_04_cert.pem";
    $pub_key_file = "../certs/identity/test_participant_03_private_key.pem";
    $sub_key_file = "../certs/identity/test_participant_04_private_key.pem";
    @gov_keys = ("AU_UA_ND_NL_NR");
    @pub_perm_files = ("permissions/permissions_test_participant_03_allowall_signed.p7s");
    @sub_perm_files = ("permissions/permissions_test_participant_04_allowall_signed.p7s");
    @topic_names = ("OD_OL_OA_OM_OD");
  } elsif ($scenario eq "SC1_sec_off_success") {
    # SC1 (join controlled domain) : unsecure participants won't check governance
    $pub_cfg_file = "unsec_base.ini";
    $sub_cfg_file = "unsec_base.ini";
    @gov_keys = ("PU_PA_ED_EL_NR");
    @pub_perm_files = ("permissions/permissions_test_participant_01_readwrite_signed.p7s");
    @sub_perm_files = ("permissions/permissions_test_participant_02_readwrite_signed.p7s");
    @topic_names = ("OD_OL_OA_OM_OD");
  } elsif ($scenario eq "SC1_sec_sub_failure") {
    # SC1 (join controlled domain) :
    #   unsecure participants won't authenticate, secure participant ignores unauthenticated
    $pub_cfg_file = "unsec_base.ini";
    @gov_keys = ("PU_PA_ED_EL_NR");
    @pub_perm_files = ("permissions/permissions_test_participant_01_readwrite_signed.p7s");
    @sub_perm_files = ("permissions/permissions_test_participant_02_readwrite_signed.p7s");
    @topic_names = ("OD_OL_OA_OM_OD");
    $pub_expect = "~16";
    $sub_expect = "~27";
  } elsif ($scenario eq "SC1_sec_pub_failure") {
    # SC1 (join controlled domain) :
    #   unsecure participants won't authenticate, secure participant ignores unauthenticated
    $sub_cfg_file = "unsec_base.ini";
    @gov_keys = ("PU_PA_ED_EL_NR");
    @pub_perm_files = ("permissions/permissions_test_participant_01_readwrite_signed.p7s");
    @sub_perm_files = ("permissions/permissions_test_participant_02_readwrite_signed.p7s");
    @topic_names = ("OD_OL_OA_OM_OD");
    $pub_expect = "~16";
    $sub_expect = "~27";
  } elsif ($scenario eq "SC1_sec_on_bad_cert_failure") {
    # SC1 (join controlled domain) : secure participants with wrong credentials fail to authenticate
    $pub_key_file =
      "../certs/identity/test_participant_02_private_key.pem"; # This won't match cert (01)
    $sub_key_file =
      "../certs/identity/test_participant_01_private_key.pem"; # This won't match cert (02)
    @gov_keys = ("PU_PA_ED_EL_NR");
    @pub_perm_files = ("permissions/permissions_test_participant_01_readwrite_signed.p7s");
    @sub_perm_files = ("permissions/permissions_test_participant_02_readwrite_signed.p7s");
    @topic_names = ("OD_OL_OA_OM_OD");
    $pub_expect = "~16";
    $sub_expect = "~27";
  } elsif ($scenario eq "SC1_sec_on_bad_perm_1_failure") {
    # SC1 (join controlled domain) : secure participants with wrong permissions fail to validate
    @gov_keys = ("PU_PA_ED_EL_NR");
    # permissions domain doesn't match governance
    @pub_perm_files = ("permissions/permissions_test_participant_01_join_wrong_signed.p7s");
    # permissions domain doesn't match governance
    @sub_perm_files = ("permissions/permissions_test_participant_02_join_wrong_signed.p7s");
    @topic_names = ("OD_OL_OA_OM_OD");
    $pub_expect = "~16";
    $sub_expect = "~27";
  } elsif ($scenario eq "SC1_sec_on_bad_perm_2_failure") {
    # SC1 (join controlled domain) :
    #   secure participants with insufficient permissions fail to pass access control checks
    @gov_keys = ("PU_PA_ED_EL_NR");
    # doesn't have permission to write
    @pub_perm_files = ("permissions/permissions_test_participant_01_join_signed.p7s");
    # doesn't have permission to read
    @sub_perm_files = ("permissions/permissions_test_participant_02_join_signed.p7s");
    @topic_names = ("OD_OL_OA_OM_OD");
    $pub_expect = "~16";
    $sub_expect = "~27";
  } elsif ($scenario eq "SC1_sec_on_success") {
    #SC1 (join controlled domain) : valid participants join and send
    @gov_keys = ("PU_PA_ED_EL_NR");
    @pub_perm_files = ("permissions/permissions_test_participant_01_readwrite_signed.p7s");
    @sub_perm_files = ("permissions/permissions_test_participant_02_readwrite_signed.p7s");
    @topic_names = ("OD_OL_OA_OM_OD");
  } elsif ($scenario eq "SC2") {
    @gov_keys = ("PU_PA_ED_EL_NR");
    @pub_perm_files = ("permissions/permissions_test_participant_01_readwrite_signed.p7s");
    @sub_perm_files = ("permissions/permissions_test_participant_02_readwrite_signed.p7s");
    @topic_names = ("OD_OL_OA_EM_OD");
  } elsif ($scenario eq "SC3") {
    @gov_keys = ("PU_PA_ED_EL_NR");
    @pub_perm_files = ("permissions/permissions_test_participant_01_readwrite_signed.p7s");
    @sub_perm_files = ("permissions/permissions_test_participant_02_readwrite_signed.p7s");
    @topic_names = ("OD_OL_OA_OM_ED");
  } elsif ($scenario eq "SC4") {
    @gov_keys = ("PU_PA_ED_EL_NR");
    @pub_perm_files = ("permissions/permissions_test_participant_01_readwrite_signed.p7s");
    @sub_perm_files = ("permissions/permissions_test_participant_02_readwrite_signed.p7s");
    @topic_names = ("OD_OL_OA_SM_OD");
  } elsif ($scenario eq "SC5") {
    @gov_keys = ("PU_PA_ED_EL_NR");
    @pub_perm_files = ("permissions/permissions_test_participant_01_readwrite_signed.p7s");
    @sub_perm_files = ("permissions/permissions_test_participant_02_readwrite_signed.p7s");
    @topic_names = ("PD_OL_OA_OM_OD");
  } elsif ($scenario eq "multiple_grants") {
    # EC certs, read/write access controls and multiple grants in a single (huge) permissions file
    $pub_cert_file = "../certs/identity/test_participant_03_cert.pem";
    $sub_cert_file = "../certs/identity/test_participant_04_cert.pem";
    $pub_key_file = "../certs/identity/test_participant_03_private_key.pem";
    $sub_key_file = "../certs/identity/test_participant_04_private_key.pem";
    @gov_keys = ("PU_PA_ED_EL_NR");
    @pub_perm_files = ("permissions/permissions_multi_p_01_02_03_04_readwrite_signed.p7s");
    @sub_perm_files = ("permissions/permissions_multi_p_01_02_03_04_readwrite_signed.p7s");
    @topic_names = ("PD_OL_RWA_EM_ED");
  } elsif ($scenario eq "TEST_8_8_5_SUCCESS") {
    @gov_keys = ("PU_PA_ND_NL_NR");
    @pub_perm_files = ("permissions/permissions_test_participant_01_write_signed.p7s");
    @sub_perm_files = ("permissions/permissions_test_participant_02_read_signed.p7s");
    @topic_names = ("OD_OL_RWA_OM_OD");
  } elsif ($scenario eq "TEST_8_8_5_FAILURE") {
    @gov_keys = ("PU_PA_ND_NL_NR");
    @pub_perm_files = ("permissions/permissions_test_participant_01_read_signed.p7s");
    @sub_perm_files = ("permissions/permissions_test_participant_02_write_signed.p7s");
    @topic_names = ("OD_OL_RWA_OM_OD");
    $pub_expect = "~15";
    $sub_expect = "~25";
  } elsif ($scenario eq "FullMsgSign") {
    @gov_keys = ("PU_PA_ED_NL_SR");
    @pub_perm_files = ("permissions/permissions_test_participant_01_readwrite_signed.p7s");
    @sub_perm_files = ("permissions/permissions_test_participant_02_readwrite_signed.p7s");
    @topic_names = ("PD_OL_OA_OM_OD");
  } elsif ($scenario eq "FullMsgEncrypt") {
    @gov_keys = ("PU_PA_ED_NL_ER");
    @pub_perm_files = ("permissions/permissions_test_participant_01_readwrite_signed.p7s");
    @sub_perm_files = ("permissions/permissions_test_participant_02_readwrite_signed.p7s");
    @topic_names = ("PD_OL_OA_OM_OD");
  } elsif ($scenario eq "FullMsgSign_SubMsgSign") {
    @gov_keys = ("PU_PA_ED_NL_SR");
    @pub_perm_files = ("permissions/permissions_test_participant_01_readwrite_signed.p7s");
    @sub_perm_files = ("permissions/permissions_test_participant_02_readwrite_signed.p7s");
    @topic_names = ("PD_OL_OA_SM_OD");
  } elsif ($scenario eq "FullMsgSign_SubMsgEncrypt") {
    @gov_keys = ("PU_PA_ED_NL_SR");
    @pub_perm_files = ("permissions/permissions_test_participant_01_readwrite_signed.p7s");
    @sub_perm_files = ("permissions/permissions_test_participant_02_readwrite_signed.p7s");
    @topic_names = ("PD_OL_OA_EM_OD");
  } elsif ($scenario eq "FullMsgEncrypt_SubMsgSign") {
    @gov_keys = ("PU_PA_ED_NL_ER");
    @pub_perm_files = ("permissions/permissions_test_participant_01_readwrite_signed.p7s");
    @sub_perm_files = ("permissions/permissions_test_participant_02_readwrite_signed.p7s");
    @topic_names = ("PD_OL_OA_SM_OD");
  } elsif ($scenario eq "FullMsgEncrypt_SubMsgEncrypt") {
    @gov_keys = ("PU_PA_ED_NL_ER");
    @pub_perm_files = ("permissions/permissions_test_participant_01_readwrite_signed.p7s");
    @sub_perm_files = ("permissions/permissions_test_participant_02_readwrite_signed.p7s");
    @topic_names = ("PD_OL_OA_EM_OD");
  } elsif ($scenario eq "FullMsgSign_PayloadEncrypt") {
    @gov_keys = ("PU_PA_ED_NL_SR");
    @pub_perm_files = ("permissions/permissions_test_participant_01_readwrite_signed.p7s");
    @sub_perm_files = ("permissions/permissions_test_participant_02_readwrite_signed.p7s");
    @topic_names = ("PD_OL_OA_OM_ED");
  } elsif ($scenario eq "FullMsgEncrypt_PayloadEncrypt") {
    @gov_keys = ("PU_PA_ED_NL_ER");
    @pub_perm_files = ("permissions/permissions_test_participant_01_readwrite_signed.p7s");
    @sub_perm_files = ("permissions/permissions_test_participant_02_readwrite_signed.p7s");
    @topic_names = ("PD_OL_OA_OM_ED");
  } elsif ($scenario eq "FullMsgSign_SubMsgSign_PayloadEncrypt") {
    @gov_keys = ("PU_PA_ED_NL_SR");
    @pub_perm_files = ("permissions/permissions_test_participant_01_readwrite_signed.p7s");
    @sub_perm_files = ("permissions/permissions_test_participant_02_readwrite_signed.p7s");
    @topic_names = ("PD_OL_OA_SM_ED");
  } elsif ($scenario eq "FullMsgSign_SubMsgEncrypt_PayloadEncrypt") {
    @gov_keys = ("PU_PA_ED_NL_SR");
    @pub_perm_files = ("permissions/permissions_test_participant_01_readwrite_signed.p7s");
    @sub_perm_files = ("permissions/permissions_test_participant_02_readwrite_signed.p7s");
    @topic_names = ("PD_OL_OA_EM_ED");
  } elsif ($scenario eq "FullMsgEncrypt_SubMsgSign_PayloadEncrypt") {
    @gov_keys = ("PU_PA_ED_NL_ER");
    @pub_perm_files = ("permissions/permissions_test_participant_01_readwrite_signed.p7s");
    @sub_perm_files = ("permissions/permissions_test_participant_02_readwrite_signed.p7s");
    @topic_names = ("PD_OL_OA_SM_ED");
  } elsif ($scenario eq "FullMsgEncrypt_SubMsgEncrypt_PayloadEncrypt") {
    @gov_keys = ("PU_PA_ED_NL_ER");
    @pub_perm_files = ("permissions/permissions_test_participant_01_readwrite_signed.p7s");
    @sub_perm_files = ("permissions/permissions_test_participant_02_readwrite_signed.p7s");
    @topic_names = ("PD_OL_OA_EM_ED");
  } elsif ($scenario eq "NetProfiling_sec_off") {
    $pub_cfg_file = "unsec_base.ini";
    $sub_cfg_file = "unsec_base.ini";
    @gov_keys = ("AU_UA_ND_NL_NR");
    @pub_perm_files = ("permissions/permissions_test_participant_01_allowall_signed.p7s");
    @sub_perm_files = ("permissions/permissions_test_participant_02_allowall_signed.p7s");
    @topic_names = ("OD_OL_OA_OM_OD");
  } elsif ($scenario eq "NetProfiling_auth_acc") {
    @gov_keys = ("PU_PA_ND_NL_NR");
    @pub_perm_files = ("permissions/permissions_test_participant_01_allowall_signed.p7s");
    @sub_perm_files = ("permissions/permissions_test_participant_02_allowall_signed.p7s");
    @topic_names = ("OD_OL_RWA_OM_OD");
  } elsif ($scenario eq "NetProfiling_encrypt") {
    @gov_keys = ("PU_PA_ED_EL_NR");
    @pub_perm_files = ("permissions/permissions_test_participant_01_allowall_signed.p7s");
    @sub_perm_files = ("permissions/permissions_test_participant_02_allowall_signed.p7s");
    @topic_names = ("PD_OL_RWA_EM_ED");
  } elsif ($scenario eq "FullMsgSign_PayloadEncrypt_Frag") {
    @gov_keys = ("PU_PA_ED_NL_SR");
    @pub_perm_files = ("permissions/permissions_test_participant_01_readwrite_signed.p7s");
    @sub_perm_files = ("permissions/permissions_test_participant_02_readwrite_signed.p7s");
    @topic_names = ("PD_OL_OA_OM_ED");
    $pub_extra_space = "100000";
  } elsif ($scenario eq "Partitions_DefaultQoS") {
    @gov_keys = ("PU_PA_ND_NL_NR");
    @pub_perm_files = ("permissions/permissions_test_participant_01_partitions_signed.p7s");
    @sub_perm_files = ("permissions/permissions_test_participant_02_partitions_signed.p7s");
    @topic_names = ("OD_OL_RWA_OM_OD");
  } elsif ($scenario eq "Partitions_Denied") {
    @gov_keys = ("PU_PA_ND_NL_NR");
    @pub_perm_files = ("permissions/permissions_test_participant_01_partitions_signed.p7s");
    @sub_perm_files = ("permissions/permissions_test_participant_02_partitions_signed.p7s");
    @topic_names = ("OD_OL_RWA_OM_OD");
    @partition = ("baz");
    $pub_expect = "~15";
    $sub_expect = "~25";
  } elsif ($scenario eq "Partitions_Match") {
    @gov_keys = ("PU_PA_ND_NL_NR");
    @pub_perm_files = ("permissions/permissions_test_participant_01_partitions_signed.p7s");
    @sub_perm_files = ("permissions/permissions_test_participant_02_partitions_signed.p7s");
    @topic_names = ("OD_OL_RWA_OM_OD");
    @partition = ("foo", "bar");
  } elsif ($scenario eq "Partitions_Reordered") {
    @gov_keys = ("PU_PA_ND_NL_NR");
    @pub_perm_files = ("permissions/permissions_test_participant_01_partitions_signed.p7s");
    @sub_perm_files = ("permissions/permissions_test_participant_02_partitions_signed.p7s");
    @topic_names = ("OD_OL_RWA_OM_OD");
    @partition = ("bar", "foo");
  } elsif ($scenario eq "Partitions_Subset") {
    @gov_keys = ("PU_PA_ND_NL_NR");
    @pub_perm_files = ("permissions/permissions_test_participant_01_partitions_signed.p7s");
    @sub_perm_files = ("permissions/permissions_test_participant_02_partitions_signed.p7s");
    @topic_names = ("OD_OL_RWA_OM_OD");
    @partition = ("bar");
  } elsif ($scenario =~ /^(un)?secure-part-user-data$/) {
    $pub_timeout = $sub_timeout = 20;
    my $secure_sub = $1 ? 0 : 1;
    @gov_keys = ("AU_UA_ND_NL_NR");
    @topic_names = ('OD_OL_OA_OM_OD');
    @pub_perm_files = ("permissions/permissions_test_participant_01_allowall_signed.p7s");
    # The publisher will set and try to protect its participant user data.
    push(@base_pub_args, "--secure-part-user-data");
    @sub_perm_files = ("permissions/permissions_test_participant_02_allowall_signed.p7s");
    push(@base_sub_args, $secure_sub ?
      # If secure, subscriber will expect the non standard behavior from the
      # publisher and be able to get the correct user data.
      "--secure-part-user-data --expect-part-user-data" :
      # Otherwise it will expect blank user data.
      "-DCPSSecurity 0 --expect-blank-part-user-data");
  } else {
    print "\nERROR: invalid scenario '$scenario'\n";
    exit 1;
  }
} else { # Not using scenarios

  sub print_using_array {
    my $what = shift;
    my $s = (scalar(@ARGV) == 1) ? '' : 's';
    print("Using $what$s: " . join(',', map {"'$_'"} @_) . "\n");
  }

  # Figure out what governance files to use
  if (scalar @gov_keys == 0) {
    print "Using governance files from governance directory.\n";
    opendir(my $gov_dh, "governance");
    @gov_keys = map {"governance/" . $_} (sort grep(/\.p7s$/,readdir($gov_dh)));
    closedir($gov_dh);

    # Filter out allow unauth + protected disc and prohibit unauth + unprotected discovery
    @gov_keys = grep(!/_AU_PA/, @gov_keys);
    @gov_keys = grep(!/_PU_UA/, @gov_keys);

    @gov_keys = grep(!/_E.*_E./, @gov_keys); # eliminate more than one encryption attribute

    @gov_keys = grep(!/_S/, @gov_keys); # eliminate signed stuff
    @gov_keys = grep(!/_SO/, @gov_keys); # eliminate origin authenticated signed stuff
    @gov_keys = grep(!/_EO/, @gov_keys); # eliminate origin authenticated encrypted stuff
  } else {
    print_using_array("governance file", @gov_keys);
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
      print_using_array("publisher permissions file", @pub_perm_files);
    }

    if (scalar @sub_perm_files == 0) {
      print "Using subscriber permissions files from permissions directory.\n";
      @sub_perm_files = @perm_files;

      #Filter permissions files to 2nd participant and full read/write
      @sub_perm_files = grep(/_test_participant_02/, @sub_perm_files);
      @sub_perm_files = grep(/_readwrite/, @sub_perm_files);

    } else {
      print_using_array("subscriber permissions file", @sub_perm_files);
    }
  } else {
    print_using_array("subscriber permissions file", @sub_perm_files);
    print_using_array("publisher permissions file", @pub_perm_files);
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
    print_using_array("topic", @topic_names);
  }
}

#open my $status_file, '>', "expected_status_results.txt";

my $total_test_count =
  scalar(@gov_keys) * scalar(@pub_perm_files) * scalar(@sub_perm_files) * scalar(@topic_names);
my $current_test_num = 0;

my $final_status = 0;

my $super_test = new PerlDDS::TestFramework();

foreach my $gov_key (@gov_keys) {
  $super_test->generate_governance($gov_key, "governance.xml.p7s");

  foreach my $pub_perm_file (@pub_perm_files) {
    foreach my $sub_perm_file (@sub_perm_files) {
      foreach my $topic_name (@topic_names) {

        my $status = 0;

        my $test = new PerlDDS::TestFramework();

        # Suppress encdec_error in Full Message Scenarios
        if ($scenario =~ /^FullMsg/) {
          $test->{dcps_security_debug} = join(',', qw/
            access_warn auth_warn auth_debug
            encdec_warn encdec_debug
            bookkeeping new_entity_warn new_entity_error
          /);
        }

        $test->{dcps_debug_level} = 4;
        $test->{dcps_transport_debug_level} = 2;
        # will manually set -DCPSConfigFile
        $test->{add_transport_config} = 0;

        my @common_args = (
          "-Governance governance.xml.p7s",
          "-Topic $topic_name",
          "-IdentityCA ../certs/identity/identity_ca_cert.pem",
          "-PermissionsCA ../certs/permissions/permissions_ca_cert.pem",
        );
        my $pub_opts = join(' ', @common_args, @base_pub_args);
        my $sub_opts = join(' ', @common_args, @base_sub_args);

        $pub_opts .= " -DCPSConfigFile $pub_cfg_file";
        $sub_opts .= " -DCPSConfigFile $sub_cfg_file";

        $pub_opts .= " -Identity $pub_cert_file";
        $sub_opts .= " -Identity $sub_cert_file";

        $pub_opts .= " -PrivateKey $pub_key_file";
        $sub_opts .= " -PrivateKey $sub_key_file";

        $pub_opts .= " -Permissions $pub_perm_file";
        $sub_opts .= " -Permissions $sub_perm_file";

        if ($pub_expect ne "0") {
          $pub_opts .= " -Expected $pub_expect";
        }

        if ($sub_expect ne "0") {
          $sub_opts .= " -Expected $sub_expect";
        }

        if ($pub_timeout ne "0") {
          $pub_opts .= " -Timeout $pub_timeout";
        }

        if ($sub_timeout ne "0") {
          $sub_opts .= " -Timeout $sub_timeout";
        }

        if ($pub_extra_space ne "0") {
          $pub_opts .= " -ExtraSpace $pub_extra_space";
        }

        for my $p (@partition) {
          $pub_opts .= " -Partition $p";
          $sub_opts .= " -Partition $p";
        }

        #print "$gov_key $pub_perm_file $sub_perm_file\n";

        $test->process("publisher", "publisher", $pub_opts);
        $test->process("subscriber", "subscriber", $sub_opts);

        $test->start_process("subscriber");
        $test->start_process("publisher");

        # start killing processes in 10 seconds
        $test->{wait_after_first_proc} = 10;
        my $status = $test->finish(30);

        #if ($status != 0) {
          $current_test_num++;

          if ($total_test_count != 1) {
            print "\ntest #$current_test_num of $total_test_count.\n";
          }

          #print "$gov_key $pub_perm_file $sub_perm_file $topic_name $status\n\n-----------\n\n";
          #print $status_file "$gov_key $pub_perm_file $sub_perm_file $topic_name $status\n";

          #exit $status;
        #}

        if ($status != 0) {
          $final_status = 1;
        }
      }
    }
  }
}

exit $final_status;
