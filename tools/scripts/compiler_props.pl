#!/usr/bin/env perl

use warnings;
use strict;

use Getopt::Long qw/GetOptions/;
use FindBin;
use lib "$FindBin::Bin/modules";

use CompilerProps;

my $usage_message =
  "usage: compiler_props.pl -h|--help\n" .
  "       compiler_props.pl COMPILER [PROP] [OPTIONS]\n" .
  "       compiler_props.pl --android-abi ABI\n" .
  "                         [--android-ndk NDK --android-api API] [PROP] [OPTIONS]\n";

my $help_message = $usage_message .
  "\n" .
  "Extract properties of a supported C++ compiler that can be ran.\n" .
  "\n" .
  "COMPILER                Compiler command to inspect.\n" .
  "PROP                    Single property to print. Prints all properties if not\n" .
  "                        given\n" .
  "\n" .
  "OPTIONS:\n" .
  "--help | -h             Prints this message\n" .
  "--debug                 Prints debug information\n" .
  "--default-cpp-std STD   Prints 1 if compiler's default C++ standard is at least\n" .
  "                        STD, else prints 0.\n" .
  "--version-at-least VER  Prints 1 if compiler's version is at least VER, else\n" .
  "                        prints 0.\n" .
  "--android-abi ABI       Android target CPU. Pass this instead of COMPILER to\n" .
  "                        try to find the Android cross-compiler.\n" .
  "--android-ndk DIR       Android NDK directory if using the NDK directly\n" .
  "--android-api API       Android API level if using the NDK directly\n";

my $debug = 0;
my $help = 0;
my $default_cpp_std = undef;
my $version_at_least = undef;
my $android_abi = undef;
my $android_ndk = undef;
my $android_api = undef;

if (!GetOptions(
  'h|help' => \$help,
  'debug' => \$debug,
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

my $compiler_command = shift;
if (!defined($compiler_command) && !defined($android_abi)) {
  print STDERR "Must pass compiler command to inspect\n";
  print STDERR $usage_message;
  exit(1);
}
my $get_prop = shift;

my $props = CompilerProps->new($compiler_command, {
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
