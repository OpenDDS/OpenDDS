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
my @pub_perm_files;
my @sub_perm_files;
my @topic_names;
my $pub_expect = "0";
my $sub_expect = "0";

GetOptions ( 'scenario=s' => \$scenario, 'gov=s' => \@gov_files, 'pub_perm=s' => \@pub_perm_files, 'sub_perm=s' => \@sub_perm_files, 'topic=s' => \@topic_names, 'pub_expect=i' => \$pub_expect, 'sub_expect=i' => \$sub_expect );

# Handle scenarios first, since they are a special case
if (!($scenario eq "")) {
  if ($scenario eq "SC0") {
    @gov_files = ("governance/governance_AU_UA_ND_NL_NR_signed.p7s");
    @pub_perm_files = ("permissions/permissions_test_participant_01_allowall_signed.p7s");
    @sub_perm_files = ("permissions/permissions_test_participant_02_allowall_signed.p7s");
    @topic_names = ("OD_OA_OM_OD");
  } elsif ($scenario eq "SC1") {
    @gov_files = ("governance/governance_PU_PA_ED_EL_NR_signed.p7s");
    @pub_perm_files = ("permissions/permissions_test_participant_01_readwrite_signed.p7s");
    @sub_perm_files = ("permissions/permissions_test_participant_02_readwrite_signed.p7s");
    @topic_names = ("OD_OA_OM_ED");
  } elsif ($scenario eq "TEST_8_8_5_SUCCESS") {
    @gov_files = ("governance/governance_PU_PA_ND_NL_NR_signed.p7s");
    @pub_perm_files = ("permissions/permissions_test_participant_01_write_signed.p7s");
    @sub_perm_files = ("permissions/permissions_test_participant_02_read_signed.p7s");
    @topic_names = ("OD_RWA_OM_OD");
  } elsif ($scenario eq "TEST_8_8_5_FAILURE") {
    @gov_files = ("governance/governance_PU_PA_ND_NL_NR_signed.p7s");
    @pub_perm_files = ("permissions/permissions_test_participant_01_read_signed.p7s");
    @sub_perm_files = ("permissions/permissions_test_participant_02_write_signed.p7s");
    @topic_names = ("OD_RWA_OM_OD");
    $pub_expect = "~13";
    $sub_expect = "~23";
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

        $pub_opts .= " -DCPSConfigFile sec_base.ini";
        $sub_opts .= " -DCPSConfigFile sec_base.ini";

        $pub_opts .= " -IdentityCA certs/identity/identity_ca_cert.pem";
        $sub_opts .= " -IdentityCA certs/identity/identity_ca_cert.pem";

        $pub_opts .= " -Identity certs/identity/test_participant_01_cert.pem";
        $sub_opts .= " -Identity certs/identity/test_participant_02_cert.pem";

        $pub_opts .= " -PrivateKey certs/identity/test_participant_01_private_key.pem";
        $sub_opts .= " -PrivateKey certs/identity/test_participant_02_private_key.pem";

        $pub_opts .= " -PermissionsCA certs/permissions/permissions_ca_cert.pem";
        $sub_opts .= " -PermissionsCA certs/permissions/permissions_ca_cert.pem";

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

        #print "$gov_file $pub_perm_file $sub_perm_file\n";

        $test->process("publisher", "publisher", $pub_opts);
        $test->process("subscriber", "subscriber", $sub_opts);

        $test->start_process("subscriber");
        $test->start_process("publisher");

        # start killing processes in 10 seconds
        $test->{wait_after_first_proc} = 10;
        my $status = $test->finish(10);

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
