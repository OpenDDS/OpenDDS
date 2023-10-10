#!/usr/bin/perl

use strict;
use warnings;

my $ace_root = shift();

sub whack_to_slash {
  my $s = shift;
  $s =~ s|\\|/|g;
  return $s;
}

sub to_cmake_scalar {
  my $val = shift;
  return 'ON' if "$val" eq '1';
  return 'OFF' if "$val" eq '0';
  return '"' . whack_to_slash($val) . '"';
}

sub to_cmake_list {
  my @list = @{$_[0]};
  return '"' . join(';', map {whack_to_slash($_)} @list) . '"';
}

sub to_cmake_value {
  my $value = shift();

  my $ref_type = ref($value);
  if ($ref_type eq 'ARRAY') {
    return to_cmake_list($value);
  }
  elsif ($ref_type eq '') {
    return to_cmake_scalar($value);
  }
  else {
    die "ERROR: to_cmake_value invalid value type, stopped";
  }
}

my %features;
my $file = "$ace_root/bin/MakeProjectCreator/config/default.features";
open(my $fh, '<', $file) or die "ERROR: Could not open $file: $!";
while (my $row = <$fh>) {
  chomp $row;
  $row =~ s/\/\/.*//;
  $row =~ s/^\s+//;
  $row =~ s/\s+$//;
  next if $row eq '';
  my ($key, $value) = split(/\s*=\s*/, $row);
  $features{$key} = $value;
}

for my $f (@ARGV) {
  my ($key, $value) = split(/=/, $f);
  $value = "1" unless defined $value;
  $features{$key} = $value;
}

my @configs = (
  {
    name => 'CXX11',
    feature => 'no_cxx11',
    inverted => 1,
  },
  {
    name => 'WCHAR',
    feature => 'uses_wchar',
  },
  {
    name => 'IPV6',
    feature => 'ipv6',
  },
  {
    name => 'SAFETY_PROFILE',
    feature => 'no_opendds_safety_profile',
    inverted => 1,
  },
  {
    name => 'VERSIONED_NAMESPACE',
    feature => 'versioned_namespace',
  },
  {
    name => 'SUPPRESS_ANYS',
    feature => 'dds_suppress_anys',
    default => 1,
  },
  {
    name => 'COVERAGE',
    feature => 'dds_non_coverage',
    inverted => 1,
  },
  {
    name => 'TAO_CORBA_E_COMPACT',
    feature => 'corba_e_compact',
  },
  {
    name => 'TAO_CORBA_E_MICRO',
    feature => 'corba_e_micro',
  },
  {
    name => 'TAO_MINIMUM_CORBA',
    feature => 'minimum_corba',
  },
  {
    name => 'TAO_IIOP',
    feature => 'tao_no_iiop',
    inverted => 1,
    default => 1,
  },
  {
    name => 'TAO_GEN_OSTREAM',
    feature => 'gen_ostream',
  },
  {
    name => 'TAO_OPTIMIZE_COLLOCATED_INVOCATIONS',
    feature => 'optimize_collocated_invocations',
    default => 1,
  },
);

my @cmake_values;
for my $config (@configs) {
  my $name = $config->{name};
  $name = uc("OPENDDS_$name");
  $name =~ s/-/_/g;
  my $inverted = $config->{inverted} // 0;
  my $default = $config->{default} // 0;
  my $enabled;
  if (exists($features{$config->{feature}})) {
    $enabled = $features{$config->{feature}} eq "1" ? !$inverted : $inverted;
  }
  else {
    $enabled = $default;
  }
  push(@cmake_values, "$name=" . to_cmake_value($enabled));
}
print(join(';', @cmake_values), "\n");
