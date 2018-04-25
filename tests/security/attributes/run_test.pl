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

use Getopt::Long;

my @gov_files;
my @perm_files;
my @topic_names;

GetOptions ( 'gov=s' => \@gov_files, 'perm=s' => \@perm_files, 'topic=s' => \@topic_names);

if (scalar @gov_files == 0) {
  opendir(my $gov_dh, "governance");
  @gov_files = map {"governance/" . $_} (sort grep(/\.p7s$/,readdir($gov_dh)));
  closedir($gov_dh);
}

# Filter out encrypted governance settings
@gov_files = grep(!/_ED_EL_/, @gov_files);

if (scalar @perm_files == 0) {
  opendir(my $perm_dh, "permissions");
  @perm_files = map {"permissions/" . $_} (sort grep(/\.p7s$/,readdir($perm_dh)));
  closedir($perm_dh);
}

# Filter out reads for publishers and writes for subscribers
my @pub_perm_files = grep(!/_read_/, @perm_files);
my @sub_perm_files = grep(!/_write_/, @perm_files);

if (scalar @topic_names == 0) {
  open my $topic_names_file, '<', "topic_names.txt";
  chomp(@topic_names = <$topic_names_file>);
  close $topic_names_file;
}

open my $status_file, '>', "expected_status_results.txt";

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

        #print "$gov_file $pub_perm_file $sub_perm_file\n";

        $test->process("publisher", "publisher", $pub_opts);
        $test->process("subscriber", "subscriber", $sub_opts);

        $test->start_process("subscriber");
        $test->start_process("publisher");

        # start killing processes in 10 seconds
        $test->{wait_after_first_proc} = 10;
        my $status = $test->finish(10);

        #if ($status != 0) {
          print "\n$gov_file $pub_perm_file $sub_perm_file $topic_name $status $test->{combined_return_codes}\n\n-----------\n\n";
          print $status_file "$gov_file $pub_perm_file $sub_perm_file $topic_name $status $test->{combined_return_codes}\n";
          #exit $status;
        #}
      }
    }
  }
}

exit 0;
