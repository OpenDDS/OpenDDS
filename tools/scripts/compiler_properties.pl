#!/usr/bin/env perl

use warnings;
use strict;

use FindBin;
use lib "$FindBin::Bin/modules";

use compiler_properties;

my $compiler_command = shift;
if (!defined($compiler_command)) {
  die("Must pass compiler command to inspect");
}
my $compiler_kind = compiler_properties::kind_from_command($compiler_command);
if (!defined($compiler_kind)) {
  die("Could not determine compiler kind of command: $compiler_command");
}
my $get_prop = shift;

my $props_hash = compiler_properties::get_props($compiler_command, $compiler_kind);

if (defined($get_prop)) {
  die ("Not such property $get_prop") if (!exists($props_hash->{$get_prop}));
  print("$props_hash->{$get_prop}\n");
}
else {
  my $props_array = compiler_properties::props_in_order($props_hash);
  for my $prop (@{$props_array}) {
    print("$prop->{name}: $prop->{value}\n");
  }
}
