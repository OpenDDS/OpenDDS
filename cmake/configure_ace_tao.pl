#!/usr/bin/perl

use strict;
use warnings;

use File::Spec;
use File::Copy qw/copy/;
use Getopt::Long qw/GetOptions/;
use Config;

use FindBin;
use lib "$FindBin::RealBin/../tools/scripts/modules";
use command_utils;

my @required_values = qw/
  mpc
  mpc-type
  ace
  tao
  config-file
/;
my @optional_values = qw/
  platform-macros-file
/;
my %is_value_required = map {$_ => 1} @required_values;
my %values = ();
my @opts = ();
for my $key (@required_values, @optional_values) {
  $values{$key} = undef;
  push(@opts, "$key=s");
}
if (!GetOptions(\%values, @opts)) {
  exit(1);
}

my $status = 0;
for my $name (keys(%values)) {
  if (defined($values{$name})) {
    print("$name: $values{$name}\n");
  }
  elsif ($is_value_required{$name}) {
    print STDERR ("Required option --$name was not passed\n");
    $status = 1;
  }
}
my $gnuace = $values{'mpc-type'} eq 'gnuace';
if ($gnuace && !defined($values{'platform-macros-file'})) {
  print STDERR ("--mpc-type gnuace requires --platform-macros-file\n");
  $status = 1;
}
exit($status) if ($status);

sub run_command {
  my $command = shift;
  return command_utils::run_command(
    $command,
    script_name => 'configure_ace_tao',
    verbose => 1,
    autodie => 1,
    @_,
  );
}

my $features_path = "$values{ace}/bin/MakeProjectCreator/config/default.features";
open(my $features_file, '>', $features_path) or die("Failed to open $features_path: $!");
for my $feature (@ARGV) {
  print("$feature\n");
  print $features_file ("$feature\n");
}
close($features_file);

my $config_path = "$values{ace}/ace/config.h";
open(my $config_file, '>', $config_path) or die("Failed to open $config_path: $!");
print $config_file (
  "#define ACE_DISABLE_MKTEMP\n" .
  "#define ACE_LACKS_READDIR_R\n" .
  "#define ACE_LACKS_TEMPNAM\n" .
  "#include \"$values{'config-file'}\"\n");
close($config_file);

if ($gnuace) {
  my $platform_macros_path = "$values{ace}/include/makeinclude/platform_macros.GNU";
  open(my $platform_macros_file, '>', $platform_macros_path)
    or die("Failed to open $platform_macros_path: $!");
  print $platform_macros_file (
    "optimize = 0\n" .
    "include \$(ACE_ROOT)/include/makeinclude/$values{'platform-macros-file'}\n");
  close($platform_macros_file);
}

$ENV{MPC_ROOT} = File::Spec->rel2abs($values{mpc});
$ENV{ACE_ROOT} = File::Spec->rel2abs($values{ace});
$ENV{TAO_ROOT} = File::Spec->rel2abs($values{tao});
my $mwc_name = 'ACE_TAO_for_OpenDDS.mwc';
my $mwc_src = "$FindBin::RealBin/../$mwc_name";
my $mwc = "$ENV{ACE_ROOT}/$mwc_name";
copy($mwc_src, $mwc) or die("Failed to copy $mwc_src to $mwc: $!");
run_command(
  [$Config{perlpath}, 'bin/mwc.pl', $mwc, '-type', $values{'mpc-type'}],
  chdir => $ENV{ACE_ROOT},
);
