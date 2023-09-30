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

    my $loc = undef;
    open(my $pf_fh, $pf) or die("Couldn't open \"$pf\": $!");
    while (my $pf_line = <$pf_fh>) {
      $pf_line =~ s/\s$//;
      if ($pf_line =~ /<OutputFile>(\$\(OutDir\))?(.*)<\/OutputFile>$/) {
        my $out_dir = $1;
        my $output_file = $2;
        $output_file =~ s/d?\.dll/d.dll/;
        $output_file =~ s/\.dll$/.lib/;
        if ($out_dir) {
          my $dir = $output_file =~ /.exe/ ? 'bin' : 'lib';
          $loc = File::Spec->catfile($values{ace}, $dir, $output_file);
        }
        else {
          $loc = File::Spec->catfile(dirname($pf), $output_file);
        }
        last;
      }
    }
    if (!defined($loc)) {
        # print STDERR ("Didn't get OutputFile from $pf\n");
        next;
    }

    # print STDERR ("$name $pf $loc\n");
    $projects{$name} = {
      name => $name,
      loc => $loc,
    };
  }
}

print(JSON::PP->new->pretty(0)->utf8->encode(\%projects));
