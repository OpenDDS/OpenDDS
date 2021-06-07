#!/usr/bin/env perl

use warnings;
use strict;

use Getopt::Long qw/GetOptions/;
use FindBin;
use lib "$FindBin::Bin/modules";

use CompilerProps;
use OsProps;

my $usage_message =
  "usage: compiler_props.pl -h|--help\n" .
  "       compiler_props.pl [OPTIONS] [PROP]\n";

my $help_message = $usage_message .
  "\n" .
  "Extract properties of a supported C++ compiler that can be ran.\n" .
  "\n" .
  "PROP                    Single property to print. Prints all properties if not\n" .
  "                        given\n" .
  "\n" .
  "OPTIONS:\n" .
  "--help | -h             Prints this message\n" .
  "--debug | -d            Prints debug information\n" .
  "--compiler | -c CMD     Compiler command to inspect. If ommited it will try to\n" .
  "                        guess. If --android-abi wasn't passed, then it will be\n" .
  "                        the \"standard\" compiler for this platform.\n" .
  "--default-cpp-std STD   Prints 1 if compiler's default C++ standard is at least\n" .
  "                        STD, else prints 0.\n" .
  "--version-at-least VER  Prints 1 if compiler's version is at least VER, else\n" .
  "                        prints 0.\n" .
  "--android-abi ABI       Android target CPU. Pass this instead of COMPILER to\n" .
  "                        try to find the Android cross-compiler.\n" .
  "--android-ndk DIR       Android NDK directory if using the NDK directly.\n" .
  "                        Requires --android-api\n" .
  "--android-api API       Android API level if using the NDK directly\n";

my $debug = 0;
my $help = 0;
my $compiler = undef;
my $default_cpp_std = undef;
my $version_at_least = undef;
my $android_abi = undef;
my $android_ndk = undef;
my $android_api = undef;

if (!GetOptions(
  'h|help' => \$help,
  'd|debug' => \$debug,
  'c|compiler=s' => \$compiler,
  'default-cpp-std=s' => \$default_cpp_std,
  'version-at-least=s' => \$version_at_least,
  'android-abi=s' => \$android_abi,
  'android-ndk=s' => \$android_ndk,
  'android-api=s' => \$android_api,
)) {
  print STDERR $usage_message;
  exit(1);
}
if ($help) {
  print($help_message);
  exit(0);
}

my $get_prop = shift;

my $os = OsProps->new({debug => $debug});

if (!defined($compiler) && !defined($android_abi)) {
  $compiler = $os->get_compiler();
  die("ERROR: Could not find compiler automatically")
    unless ($compiler);
}

my $props = CompilerProps->new($compiler, {
  os_props => $os,
  debug => $debug,
  android_abi => $android_abi,
  android_ndk => $android_ndk,
  android_api => $android_api,
});

if (defined($get_prop)) {
  print($props->require($get_prop), "\n");
}
elsif (defined($default_cpp_std)) {
  print($props->default_cpp_std_is_at_least($default_cpp_std), "\n");
}
elsif (defined($version_at_least)) {
  print($props->version_at_least($version_at_least), "\n");
}
else {
  $props->log();
}
