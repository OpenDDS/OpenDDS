#!/usr/bin/perl

use strict;
use warnings;

use File::Spec;
use Cwd qw/realpath/;
use JSON::PP;
use Getopt::Long qw/GetOptions/;

my @required_values = qw/
  workspace
  loc-base
  ace
  tao
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

my $make = 'make';
my $gnumf = 'GNUmakefile';

sub get_projects {
  my %projects;
  open(my $fh, '-|', "$make --no-print-directory -f $gnumf project_name_list") or die("get_projects make: $!");
  while (my $project = <$fh>) {
    $project =~ s/\s//;
    my $mf = "$gnumf.$project";
    $mf =~ s/-target$//;
    # print("$project $mf\n");
    @projects{$project} = {
      name => $project,
      mf => $mf,
      deps => undef,
      dir => undef,
      loc => undef,
    };
  }
  return \%projects;
}

sub scrape_makefile {
  my $mf = shift();
  my $projects = shift();
  my $project = shift();

  my $is_workspace = !defined($project);

  open(my $fh, '-|', "$make -npRrq -f $mf : 2>/dev/null") or die("$!");
  while (my $line = <$fh>) {
    $line =~ s/\s$//;
    # print("$line\n");
    if ($is_workspace) {
      # Looking for lines like:
      #   SmartProxies: ACE TAO_IDL_EXE TAO Codeset
      #     $(KEEP_GOING)@cd tao/SmartProxies && $(MAKE) -f GNUmakefile.SmartProxies all
      # Or:
      #   TAO: TAO_Core_idl
      #     $(KEEP_GOING)@$(MAKE) -f GNUmakefile.TAO all
      if ($line =~ /^([\w-]+):(?: (.+))?$/) {
        die("Unexpected rule on line $.") if (defined($project));
        if (exists($projects->{$1})) {
          $project = $projects->{$1};
          my @deps;
          if (defined($2)) {
            @deps = split(/ /, $2);
          }
          $project->{deps} = \@deps;
        }
      }
      elsif (defined($project)) {
        if ($line =~ /@(?:cd (.+) && )?\$\(MAKE\) -f $project->{mf}/) {
          die("Unexpected makefile on line $.") if (defined($project->{dir}));
          my $dir = $1 // '.';
          $project->{dir} = realpath($dir);
        }
        elsif (length($line) == 0) {
          die("Unexpected blank line on line $.") if (!defined($project->{dir}));
          $project = undef;
        }
      }
    }
    elsif ($line =~ /^build.local:(?: (.+))?$/) {
      if (!defined($1)) {
        # Remove without a build.local
        delete($projects->{$project->{name}});
        return;
      }
      $project->{loc} = File::Spec->abs2rel("$project->{dir}/$1", $values{'loc-base'});
    }
  }
}

$ENV{ACE_ROOT} = $values{ace};
$ENV{TAO_ROOT} = $values{tao};

chdir($values{workspace});
my $projects = get_projects();
scrape_makefile($gnumf, $projects);
for my $project (values(%{$projects})) {
  die("$project->{name} is missing dir") if (!defined($project->{dir}));
  chdir($project->{dir}) or die("Couldn't cd for $project->{name} to $project->{dir}: $!");
  scrape_makefile("$project->{mf}", $projects, $project);
}

# Remove deps without a build.local removed above
for my $project (values(%{$projects})) {
  my @deps = grep { exists($projects->{$_}) } @{$project->{deps}};
  $project->{deps} = \@deps;
}
print(JSON::PP->new->pretty(0)->utf8->encode($projects));
