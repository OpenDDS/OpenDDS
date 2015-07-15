use strict;
use warnings;
use Date::Format;

$ENV{TZ} = "UTC";

sub usage {
  return "gitrelease.pl <version> [<remote>] [<stepnum>] [opts]\n" .
         "  version: release version in a.b or a.b.c notation\n" .
         "   remote: valid git remote for OpenDDS\n" .
         "   opts: --list   to not make changes, perform dry run\n" .
         "         --remedy to remediate problems where possible\n" .
         "         --force  to progress where possible\n"
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
  my $settings = shift();
  my $version = $settings->{version};
  print "Updating VERSION file for $version\n";
  my $timestamp = $settings->{timestamp};
  my $outline = "This is OpenDDS version $version, released $timestamp";
  my $corrected = 0;
  open(VERSION, "+< VERSION")                 or die "Opening: $!";
  my $out = "";
  
  while (<VERSION>) {
    s/This is OpenDDS version [^,]+, released (.*)/$outline/;
      $out .= $_;
      $corrected = 1;
  }
  seek(VERSION,0,0)                        or die "Seeking: $!";
  print VERSION $out                       or die "Printing: $!";
  truncate(VERSION,tell(VERSION))          or die "Truncating: $!";
  close(VERSION)                           or die "Closing: $!";
  return $corrected;
}
############################################################################
sub verify_update_news_file {
  my $settings = shift();
  my $version = $settings->{version};
  my $status = open(NEWS, 'NEWS');
  my $metaversion = quotemeta($version);
  my $has_version = 0;
  my $corrected_features = 1;
  my $corrected_fixes = 1;
  while (<NEWS>) {
    if ($_ =~ /Version $metaversion of OpenDDS\./) {
      $has_version = 1;
    } elsif ($_ =~ /Add your features here/) {
      $corrected_features = 0;
    } elsif ($_ =~ /Add your fixes here/) {
      $corrected_fixes = 0;
    }
  }
  close(NEWS);

  return ($has_version && $corrected_features && $corrected_fixes);
}

sub message_update_news_file {
  return "NEWS file needs updating with current version release notes"
}

sub remedy_update_news_file {
  my $settings = shift();
  my $version = $settings->{version};
  print "Updating NEWS file for $version\n";
  my $timestamp = $settings->{timestamp};
  my $outline = "This is OpenDDS version $version, released $timestamp";
  my $corrected = 0;
  open(NEWS, "+< NEWS")                 or die "Opening: $!";
  my $out = "Version $version of OpenDDS.\n" . <<"ENDOUT";

Additions:
  Add your features here 

Fixes:
  Add your fixes here

ENDOUT

  $out .= join("", <NEWS>);
  seek(NEWS,0,0)                        or die "Seeking: $!";
  print NEWS $out                       or die "Printing: $!";
  truncate(NEWS,tell(NEWS))          or die "Truncating: $!";
  close(NEWS)                           or die "Closing: $!";
  return $corrected;
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
    # remedy  => sub{remedy_git_status_clean(@_)}
  },
  {
    title   => 'Update VERSION',
    verify  => sub{verify_update_version_file(@_)},
    message => sub{message_update_version_file(@_)},
    remedy  => sub{remedy_update_version_file(@_)}
  },
  {
    title   => 'Update NEWS',
    verify  => sub{verify_update_news_file(@_)},
    message => sub{message_update_news_file(@_)},
    remedy  => sub{remedy_update_news_file(@_)}
  },
  {
    title => 'Update Version.h',
    verify  => sub{verify_update_version_h_file(@_)},
    message => sub{message_update_version_h_file(@_)},
    remedy  => sub{remedy_update_version_h_file(@_)}
  },
  {
    title => 'Update PROBLEM-REPORT-FORM',
    verify  => sub{verify_update_prf_file(@_)},
    message => sub{message_update_prf_file(@_)},
    remedy  => sub{remedy_update_prf_file(@_)}
  }
);

my @t = gmtime;

sub any_arg_is {
  my $match = shift;
  foreach my $arg (@ARGV) {
    if ($arg eq $match) {
      return 1;
    }
  }
  return 0;
}

sub numeric_arg {
  foreach my $arg (@ARGV[1..$#ARGV]) {
    if ($arg =~ /[0-9]+/
      return atoi $arg;
    }
  }
  return 0;
}

my %settings = (
  list      => any_arg_is("--list"),
  remedy    => any_arg_is("--remedy"),
  force     => any_arg_is("--force"),
  only_step => numeric_arg(),
  version   => $ARGV[0],
  remote    => $ARGV[1] || "origin",
  timestamp => strftime("%a %b %e %T %Z %Y", @t),
  expected  => 'git@github.com:objectcomputing/OpenDDS.git'
);

my $step_count = 0;

sub run_step {
  print "$step_count: $step->{title}\n";
  my ($step_count, $step) = @_;
  # Run the verification
  if (!$step->{verify}(\%settings)) {
    
    # Failed
    my $message = $step->{message}(\%settings);
    if ($settings{list}) {
      print " !!!! $message\n";
    } elsif ($settings{remedy} && $step->{remedy}) {
      $step->{remedy}(\%settings);
      # Reverify
      if (!$step->{verify}(\%settings)) {
        die " !!!! Remediation did not complete step\n";
      }
    } elsif ($settings{force}) {
    } else {
      die " !!!! $message";
    }
  }

}

for my $step (@release_steps) {
  ++$step_count; 
  run_step($step_count, $step);
}

