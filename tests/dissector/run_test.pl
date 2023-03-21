#!/usr/bin/env perl

use strict;
use warnings;

use File::Copy qw/cp/;
use File::Basename qw/fileparse/;
use File::Path qw/make_path/;
use Getopt::Long;

use FindBin;
use lib "$FindBin::Bin/../../tools/scripts/modules/";
use command_utils;
use version_utils;

# Just needed to get rid of auto_run_test options
use lib "$ENV{ACE_ROOT}/bin";
use lib "$ENV{DDS_ROOT}/bin";
use PerlDDS::Run_Test;

my $no_install = 0;
my $help = 0;
my $tshark_cmd = $ENV{TSHARK} // 'tshark';
my $xdg_config_home = $ENV{XDG_CONFIG_HOME} // "$ENV{HOME}/.config";
my $pcap = "$FindBin::Bin/messenger.pcap";
$ENV{OPENDDS_DISSECTORS} = $FindBin::Bin;

my $help_message = "usage: run_test.pl [-h|--help] [--no-install]\n";
if (!GetOptions(
  "no-install!" => \$no_install,
  "tshark=s" => \$tshark_cmd,
  "help|h!" => \$help,
)) {
  print STDERR ("ERROR: Invalid options(s)\n$help_message");
  exit 1;
}
if (scalar(grep {length($_)} @ARGV)) {
  print STDERR ("ERROR: Invalid positional argument(s) passed: ", join(' ', @ARGV), "\n");
  exit 1;
}
if ($help) {
  print $help_message;
  exit 0;
}

my $tshark_stdout;
sub tshark {
  my $failed = command_utils::run_command(
    [$tshark_cmd, @_],
    capture => {stdout => \$tshark_stdout},
    verbose => 1,
  );
  if (length($tshark_stdout) == 0) {
    print("(No stdout)\n");
  }
  else {
    print(
      "begin stdout ------------------------------------------------------------------\n",
      $tshark_stdout,
      "end stdout --------------------------------------------------------------------\n",
    );
  }
  return $failed;
}

my $install_dissector = !$no_install;
my $dissector_base_name = 'OpenDDS_Dissector';
my $dissector_test_name = "${dissector_base_name}_for_test";
my $dissector_name = $install_dissector ? $dissector_test_name : $dissector_base_name;

sub copy_dissector {
  my $src_dir = shift;
  my $dest_dir = shift;
  my $found = 0;
  my $path;
  for my $filename (@_) {
    $path = "$src_dir/$filename";
    if (-f $path) {
      $found = 1;
      last;
    }
  }
  if ($found) {
    my ($name, $parent, $suffix) = fileparse($path, qr"\..[^.]*$");
    my $dest = "$dest_dir/$dissector_name$suffix";
    print("Copying $path to $dest\n");
    cp($path, $dest) or die("Copy $path to $dest failed: $!");
    return $dest;
  }
  else {
    die("ERROR: Couldn't find dissector");
  }
}

my $failed = 0;

sub plugin_installed {
  my $plugin_name = shift;
  my $failed = 0;
  if (tshark('-G', 'plugins')) {
    die("ERROR: Failed to check plugins");
  }
  my @installed = ();
  my $name = quotemeta($plugin_name);
  while ($tshark_stdout =~ /^($name\S*)\s/mg) {
    push(@installed, $1);
  }
  return join(', ', @installed);
}

print("Get wireshark version =========================================================\n");
my $ws_version;
my $ws_version_str;
if (tshark('-v')) { # --version doesn't exist in Wireshark 1.12
  die("ERROR: Could get tshark version because of exit status");
}
elsif ($tshark_stdout =~ /(\d+\.\d+.\d+)/) {
  $ws_version = parse_version($1);
  $ws_version_str = $ws_version->{series_string};
  print("Version: $ws_version_str\n");
}
else {
  die("ERROR: Could get tshark version because it wasn't found");
}

if ($install_dissector) {
  print("Make sure a dissector isn't already installed =================================\n");
  my $installed_string = plugin_installed($dissector_base_name);
  if ($installed_string) {
    print STDERR ("ERROR: There are plugins already installed: $installed_string\n");
    $failed = 1;
  }

  print("Install dissector plugin ======================================================\n");
  my $dissector_dir = "$FindBin::Bin/../../tools/dissector/";
  my $plugin_dir;
  if (version_greater_equal($ws_version, "2.5.0")) {
    $plugin_dir = "$ENV{HOME}/.local/lib/wireshark/plugins/$ws_version_str/epan";
  }
  elsif (version_greater_equal($ws_version, "2.4.0")) {
    $plugin_dir = "$xdg_config_home/wireshark/plugins"
  }
  else {
    $plugin_dir = "$ENV{HOME}/.wireshark/plugins";
  }
  print("Plugin path is \"$plugin_dir\"\n");
  make_path($plugin_dir);
  my $dissector_path = copy_dissector($dissector_dir, $plugin_dir, "OpenDDS_Dissector.so");

  END {
    if ($install_dissector && -f $dissector_path) {
      print("Deleting $dissector_path...\n");
      unlink($dissector_path);
    }
  }
}

print("Verify dissector plugin is installed ==========================================\n");
if (plugin_installed($dissector_name)) {
  print("Looks like $dissector_name was installed\n");
}
else {
  print STDERR ("ERROR: Could not verify $dissector_name was installed\n");
  $failed = 1;
}

print("Filter samples ================================================================\n");
my $filter = 'opendds.sample.publication[14:2] == 01:02 && opendds.sample.id == SAMPLE_DATA';
if (tshark('-r', $pcap, '-T', 'text', $filter)) {
  $failed = 1;
}

print("Dissect and filter sample payload contents ====================================\n");
my $field = 'opendds.sample.payload.Messenger.Message.subject_id';
if (tshark('-r', $pcap, '-T', 'fields', '-e', $field, "$field < 100")) {
  $failed = 1;
}
my $value = '99';
if ($tshark_stdout =~ /$value/) {
  print("Found $value\n");
}
else {
  print STDERR ("ERROR: Could not find $value in the above output\n");
  $failed = 1;
}

exit($failed);
