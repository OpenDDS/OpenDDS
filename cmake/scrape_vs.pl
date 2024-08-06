#!/usr/bin/perl

use strict;
use warnings;

use File::Spec;
use File::Basename;
use Cwd qw/realpath/;
use JSON::PP;
use Getopt::Long qw/GetOptions/;

my @required_values = qw/
  sln
  ace
/;
my @optional_values = qw/
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
  if (!defined($values{$name}) && $is_value_required{$name}) {
    print STDERR ("Required option --$name was not passed\n");
    $status = 1;
  }
  $values{$name} = realpath($values{$name});
}
exit($status) if ($status);

my %projects;

open(my $fh, $values{sln}) or die("Couldn't open \"$values{sln}\": $!");
while (my $line = <$fh>) {
  $line =~ s/\s$//;
  if ($line =~ /Project\("\{[^}]+\}"\) = "([^)]+)", "([^"]+)",.*/) {
    my $name = $1;
    my $pf = File::Spec->catfile(dirname($values{sln}), $2);

    my %configs;
    my $config;
    open(my $pf_fh, $pf) or die("Couldn't open \"$pf\": $!");
    while (my $pf_line = <$pf_fh>) {
      $pf_line =~ s/\s$//;
      if ($pf_line =~ /<ItemDefinitionGroup Condition="'\$\(Configuration\)\|\$\(Platform\)'=='(\w+)\|\w+'">$/) {
        $config = $1;
      }
      elsif ($pf_line =~ /<OutputFile>(\$\(\w+\))?(.*)<\/OutputFile>$/) {
        my $base_var = $1;
        my $output_file = $2;
        $output_file =~ s/\.dll$/.lib/;
        my $loc;
        if ($base_var) {
          if ($base_var eq '$(OutDir)') {
            $loc = File::Spec->catfile(
              $values{ace}, $output_file =~ /.exe/ ? 'bin' : 'lib', $output_file);
          }
          elsif ($base_var eq '$(ACE_ROOT)') {
            $loc = File::Spec->catfile($values{ace}, $output_file);
          }
          else {
            die("Unexpected name $base_var on $pf:$.");
          }
        }
        else {
          $loc = File::Spec->catfile(dirname($pf), $output_file);
        }

        if ($loc && $config) {
          $configs{$config} = $loc;
          $config = undef;
        }
      }
    }
    if (!%configs) {
      next;
    }

    my @locs = values(%configs);
    my $loc = $locs[0];
    if (@locs == grep { $_ eq $loc } @locs) {
      # The locations are the same for each config
      $projects{$name} = {
        name => $name,
        loc => $loc,
      };
    }
    else {
      $projects{$name} = {
        name => $name,
        configs => \%configs,
      };
    }
  }
}

print(JSON::PP->new->pretty(0)->utf8->encode(\%projects));
