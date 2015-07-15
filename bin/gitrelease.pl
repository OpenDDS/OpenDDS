use strict;
use warnings;

sub usage {
  return "gitrelease.pl <version> [<remote>] [opts]\n" .
         "  version: release version in a.b or a.b.c notation\n" .
         "   remote: valid git remote for OpenDDS\n" .
         "   opts: --force to force and remediate problems\n"
}

###########################################################################
sub verify_version {
  my $settings = shift();
  my $version = $settings->{version} || "";
  if ("$version" =~ /([0-9])+\.([0-9])+([0-9]+)?/) {
    $settings->{major_version} = $1;
    $settings->{minor_version} = $2;
    if ($3) {
      $settings->{micro_version} = $3;
    }
    return 1;
  } else {
    return 0;
  }
}
 
sub message_version {
  usage();
}

############################################################################
sub verify_git_remote {
  my $settings = shift();
  my $remote = $settings->{remote};
  my $url = "";
  open(GITREMOTE, "git remote show $remote|");
  while (<GITREMOTE>) {
    if ($_ =~ /Push *URL: *(.*)$/) {
      $url = $1;
      last;
    }
  }
  close(GITREMOTE);
  return ($url eq $settings->{expected});
}

sub message_git_remote {
  my $settings = shift;
  my $remote = $settings->{remote};
  return "Remote $remote does not match expected URL $settings->{expected},\n" .
         "rerun and specifiy remote";
}

############################################################################
sub verify_git_status_clean {
  my $clean = 1;
  my $status = open(GITSTATUS, 'git status -s|');
  while (<GITSTATUS>) {
    $clean = 0;
    last;
  }
  close(GITSTATUS);

  return $clean;
}

sub message_git_status_clean {
  return "The working directory is not clean.  Run git clean before continuing."
}

sub remedy_git_status_clean {
}

############################################################################
sub verify_update_version_file {
  my $settings = shift();
  my $version = $settings->{version};
  my $correct = 0;
  my $status = open(VERSION, 'VERSION');
  my $metaversion = quotemeta($version);
  while (<VERSION>) {
    if ($_ =~ /This is OpenDDS version $metaversion, released/) {
      $correct = 1;
      last;
    }
  }
  close(VERSION);

  return $correct;
}

sub message_update_version_file {
  return "VERSION file needs updating with current version"
}

sub remedy_update_version_file {
}
############################################################################
sub verify_update_news_file {
  my $settings = shift();
  my $version = $settings->{version};
  my $lines = 0;
  my $status = open(NEWS, 'NEWS');
  my $metaversion = quotemeta($version);
  while (<NEWS>) {
    if ($_ =~ /Version $metaversion of OpenDDS\./) {
      $lines = 1;
    } elsif ($_ =~ /Version .* of OpenDDS\./) {
      if ($lines) {
        # Done counting
        last;
      }
    } elsif ($lines) {
      # Count lines
      ++$lines;
    }
  }
  close(NEWS);

  return ($lines > 10);
}

sub message_update_news_file {
  return "NEWS file needs updating with current version release notes"
}
############################################################################
sub verify_update_version_h_file {
  my $settings = shift();
  my $version = $settings->{version};
  my $matched_major  = 0;
  my $matched_minor  = 0;
  my $matched_micro  = $settings->{matched_micro} ? 0 : 1;
  my $matched_version = 0;
  my $status = open(VERSION_H, 'dds/Version.h');
  my $metaversion = quotemeta($version);

  while (<VERSION_H>) {
    if ($_ =~ /^#define DDS_MAJOR_VERSION $settings->{major_version}$/) {
      ++$matched_major;
    } elsif ($_ =~ /^#define DDS_MINOR_VERSION $settings->{minor_version}$/) {
      ++$matched_minor;
    } elsif ($settings->{micro_version} && 
            ($_ =~ /^#define DDS_MICRO_VERSION $settings->{micro_version}$/)) {
      ++$matched_micro;
    } elsif ($_ =~ /^#define DDS_VERSION "$metaversion"$/) {
      ++$matched_version;
    }
  }
  close(VERSION_H);

  return (($matched_major == 1) && ($matched_minor   == 1) &&
          ($matched_micro == 1) && ($matched_version == 1));
}

sub message_update_version_h_file {
  return "dds/Version.h file needs updating with current version"
}

sub remedy_update_version_h_file {
}
############################################################################
sub verify_update_prf_file {
  my $settings = shift();
  my $version = $settings->{version};
  my $matched_header  = 0;
  my $matched_version = 0;
  my $status = open(PROBLEM_REPORT_FORM, 'PROBLEM-REPORT-FORM');
  my $metaversion = quotemeta($version);

  while (<PROBLEM_REPORT_FORM>) {
    if ($_ =~ /^This is OpenDDS version $metaversion, released/) {
      ++$matched_header;
    } elsif ($_ =~ /OpenDDS VERSION: $metaversion$/) {
      ++$matched_version;
    }
  }
  close(PROBLEM_REPORT_FORM);

  return (($matched_header == 1) && ($matched_version == 1));
}

sub message_update_prf_file {
  return "PROBLEM-REPORT-FORM file needs updating with current version"
}

sub remedy_update_prf_file {
}
############################################################################

my @release_steps = (
  {
    title   => 'Verify version arg',
    verify  => sub{verify_version(@_)},
    message => sub{message_version(@_)},
  },
  {
    title   => 'Verify remote arg',
    verify  => sub{verify_git_remote(@_)},
    message => sub{message_git_remote(@_)},
  },
  {
    title   => 'Verify git status is clean',
    verify  => sub{verify_git_status_clean(@_)},
    message => sub{message_git_status_clean(@_)},
    force   => sub{force_git_status_clean(@_)}
  },
  {
    title  => 'Update VERSION',
    verify  => sub{verify_update_version_file(@_)},
    message => sub{message_update_version_file(@_)},
    force   => sub{force_update_version_file(@_)}
  },
  {
    title => 'Update NEWS',
    verify  => sub{verify_update_news_file(@_)},
    message => sub{message_update_news_file(@_)}
  },
  {
    title => 'Update Version.h',
    verify  => sub{verify_update_version_h_file(@_)},
    message => sub{message_update_version_h_file(@_)},
    force   => sub{force_update_version_h_file(@_)}
  },
  {
    title => 'Update PROBLEM-REPORT-FORM',
    verify  => sub{verify_update_prf_file(@_)},
    message => sub{message_update_prf_file(@_)},
    force   => sub{force_update_prf_file(@_)}
  }
);

my %settings = (
  version => $ARGV[0],
  remote  => $ARGV[1] || "origin",
  expected => 'git@github.com:objectcomputing/OpenDDS.git'
);

my $count = 0;

for my $step (@release_steps) {
  ++$count; 
  print "$count: $step->{title}\n";
  # Run the verification
  if (!$step->{verify}(\%settings)) {
    
    # Failed
    my $message = $step->{message}(\%settings);
    print " !!!! $message\n";
    # die; unless --dry run
  }
}

