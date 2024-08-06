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
  src
  ace
  tao
  config-file
/;
my @optional_values = qw/
  workspace-file
  platform-macros-file
  static
  compiler
/;
my %is_value_required = map {$_ => 1} @required_values;
my %values = ();
my @opts = ();
for my $key (@required_values, @optional_values) {
  $values{$key} = undef;
  push(@opts, "$key=s");
}
push(@opts, "macro-line=s@");
my %env;
push(@opts, "env=s%" => %env);
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

sub read_file {
  my $path = shift();

  open(my $file, $path) or die("Failed to open $path: $!");
  my $contents = do { local $/; <$file> };
  close($file);
  return $contents;
}

my $features_path = "$values{ace}/bin/MakeProjectCreator/config/default.features";
open(my $features_file, '>', $features_path) or die("Failed to open $features_path: $!");
for my $feature (@ARGV) {
  print("$feature\n");
  print $features_file ("$feature\n");
}
close($features_file);

my $config_path = "$values{ace}/ace/config.h";
my $config_file_should_be =
  "#define ACE_DISABLE_MKTEMP\n" .
  "#define ACE_LACKS_READDIR_R\n" .
  "#define ACE_LACKS_TEMPNAM\n" .
  "#include \"$values{'config-file'}\"\n";
# Always writing the config.h file will cause rebuilds, so don't do it unless
# we actually need to change it.
if (!-f $config_path || read_file($config_path) ne $config_file_should_be) {
  open(my $config_file, '>', $config_path) or die("Failed to open $config_path: $!");
  print $config_file ($config_file_should_be);
  close($config_file);
}

if ($gnuace) {
  my $platform_macros_path = "$values{ace}/include/makeinclude/platform_macros.GNU";
  open(my $platform_macros_file, '>', $platform_macros_path)
    or die("Failed to open $platform_macros_path: $!");
  if ($values{compiler}) {
    for my $var ('CC', 'CXX', 'LD') {
      print $platform_macros_file ("$var = $values{compiler}\n");
    }
  }
  print $platform_macros_file ("CCFLAGS += \$(opendds_cmake_std)\n");
  if ($values{'macro-line'}) {
    for my $line (@{$values{'macro-line'}}) {
      print $platform_macros_file ("$line\n");
    }
  }
  print $platform_macros_file (
    "include \$(ACE_ROOT)/include/makeinclude/$values{'platform-macros-file'}\n");
  close($platform_macros_file);
}

$ENV{MPC_ROOT} = File::Spec->rel2abs($values{mpc});
$ENV{ACE_ROOT} = File::Spec->rel2abs($values{ace});
$ENV{TAO_ROOT} = File::Spec->rel2abs($values{tao});
if ($values{'env'}) {
  for my $name (keys(%{$values{'env'}})) {
    my $value = $values{env}->{$name};
    print("env: $name=$value\n");
    $ENV{$name} = $value;
  }
}
my $mwc_name = 'ACE_TAO_for_OpenDDS.mwc';
my $mwc_src = $values{'workspace-file'} // "$FindBin::RealBin/../$mwc_name";
my $mwc = "$values{src}/$mwc_name";
copy($mwc_src, $mwc) or die("Failed to copy $mwc_src to $mwc: $!");
my $cmd = [$Config{perlpath}, "$ENV{ACE_ROOT}/bin/mwc.pl", $mwc, '-type', $values{'mpc-type'}];
if ($values{static}) {
  push(@{$cmd}, '-static');
}
run_command($cmd, chdir => $values{src});
