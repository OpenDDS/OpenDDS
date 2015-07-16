use strict;
use warnings;
use Date::Format;

$ENV{TZ} = "UTC";

sub usage {
  return "gitrelease.pl <version> [<remote>] [<stepnum>] [opts]\n" .
         "    version:  release version in a.b or a.b.c notation\n" .
         "     remote:  valid git remote for OpenDDS (default: origin)\n" .
         "    stepnum:  # of individual step to run (default: all)\n" .
         "   opts: --list      just show steps (default check)\n" .
         "         --remedy    remediate problems where possible\n" .
         "         --force     don't exit at first error\n"
}

###########################################################################
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
  print "  >> Updating VERSION file for $version\n";
  my $timestamp = $settings->{timestamp};
  my $outline = "This is OpenDDS version $version, released $timestamp";
  my $corrected = 0;
  open(VERSION, "+< VERSION")                 or die "Opening: $!";
  my $out = "";

  while (<VERSION>) {
    if (s/This is OpenDDS version [^,]+, released (.*)/$outline/) {
      $corrected = 1;
    }
    $out .= $_;
  }
  seek(VERSION,0,0)                        or die "Seeking: $!";
  print VERSION $out                       or die "Printing: $!";
  truncate(VERSION,tell(VERSION))          or die "Truncating: $!";
  close(VERSION)                           or die "Closing: $!";
  return $corrected;
}
############################################################################
sub verify_news_file_section {
  my $settings = shift();
  my $version = $settings->{version};
  my $status = open(NEWS, 'NEWS');
  my $metaversion = quotemeta($version);
  my $has_version = 0;
  while (<NEWS>) {
    if ($_ =~ /Version $metaversion of OpenDDS\./) {
      $has_version = 1;
    }
  }
  close(NEWS);

  return ($has_version);
}

sub message_news_file_section {
  my $settings = shift();
  my $version = $settings->{version};
  return "NEWS file release $version section needs updating";
}

sub remedy_news_file_section {
  my $settings = shift();
  my $version = $settings->{version};
  print "  >> Adding $version section to NEWS\n";
  print "  !! Manual update to NEWS needed\n";
  my $timestamp = $settings->{timestamp};
  my $outline = "This is OpenDDS version $version, released $timestamp";
  open(NEWS, "+< NEWS")                 or die "Opening: $!";
  my $out = "Version $version of OpenDDS.\n" . <<"ENDOUT";

Additions:
  TODO: Add your features here

Fixes:
  TODO: Add your fixes here

ENDOUT

  $out .= join("", <NEWS>);
  seek(NEWS,0,0)                        or die "Seeking: $!";
  print NEWS $out                       or die "Printing: $!";
  truncate(NEWS,tell(NEWS))          or die "Truncating: $!";
  close(NEWS)                           or die "Closing: $!";
  return 1;
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
    } elsif ($_ =~ /TODO: Add your features here/) {
      $corrected_features = 0;
    } elsif ($_ =~ /TODO: Add your fixes here/) {
      $corrected_fixes = 0;
    }
  }
  close(NEWS);

  return ($has_version && $corrected_features && $corrected_fixes);
}

sub message_update_news_file {
  return "NEWS file needs updating with current version release notes";
}
############################################################################
sub verify_update_version_h_file {
  my $settings = shift();
  my $version = $settings->{version};
  my $matched_major  = 0;
  my $matched_minor  = 0;
  my $matched_micro  = 0;
  my $matched_version = 0;
  my $status = open(VERSION_H, 'dds/Version.h');
  my $metaversion = quotemeta($version);

  while (<VERSION_H>) {
    if ($_ =~ /^#define DDS_MAJOR_VERSION $settings->{major_version}$/) {
      ++$matched_major;
    } elsif ($_ =~ /^#define DDS_MINOR_VERSION $settings->{minor_version}$/) {
      ++$matched_minor;
    } elsif ($_ =~ /^#define DDS_MICRO_VERSION $settings->{micro_version}$/) {
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
  my $settings = shift();
  my $version = $settings->{version};
  print "  >> Updating dds/Version.h file for $version\n";
  my $corrected_major  = 0;
  my $corrected_minor  = 0;
  my $corrected_micro  = 0;
  my $corrected_version = 0;
  my $major_line = "#define DDS_MAJOR_VERSION $settings->{major_version}";
  my $minor_line = "#define DDS_MINOR_VERSION $settings->{minor_version}";
  my $micro_line = "#define DDS_MICRO_VERSION $settings->{micro_version}";
  my $version_line = "#define DDS_VERSION \"$settings->{version}\"";

  open(VERSION_H, "+< dds/Version.h")                 or die "Opening: $!";

  my $out = "";

  while (<VERSION_H>) {
    if (s/^#define DDS_MAJOR_VERSION +[0-9]+ *$/$major_line/) {
      ++$corrected_major;
    } elsif (s/^#define DDS_MINOR_VERSION +[0-9]+ *$/$minor_line/) {
      ++$corrected_minor;
    } elsif (s/^#define DDS_MICRO_VERSION +[0-9]+ *$/$micro_line/) {
      ++$corrected_micro;
    } elsif (s/^#define DDS_VERSION ".*" *$/$version_line/) {
      ++$corrected_version;
    }
    $out .= $_;
  }
  seek(VERSION_H,0,0)                        or die "Seeking: $!";
  print VERSION_H $out                       or die "Printing: $!";
  truncate(VERSION_H,tell(VERSION_H))        or die "Truncating: $!";
  close(VERSION_H)                           or die "Closing: $!";

  return (($corrected_major == 1) && ($corrected_minor   == 1) &&
          ($corrected_micro == 1) && ($corrected_version == 1));
}
############################################################################
sub verify_update_prf_file {
  my $settings = shift();
  my $version = $settings->{version};
  my $matched_header  = 0;
  my $matched_version = 0;
  my $status = open(PRF, 'PROBLEM-REPORT-FORM');
  my $metaversion = quotemeta($version);

  while (<PRF>) {
    if ($_ =~ /^This is OpenDDS version $metaversion, released/) {
      ++$matched_header;
    } elsif ($_ =~ /OpenDDS VERSION: $metaversion$/) {
      ++$matched_version;
    }
  }
  close(PRF);

  return (($matched_header == 1) && ($matched_version == 1));
}

sub message_update_prf_file {
  return "PROBLEM-REPORT-FORM file needs updating with current version"
}

sub remedy_update_prf_file {
  my $settings = shift();
  my $version = $settings->{version};
  print "  >> Updating PROBLEM-REPORT-FORM file for $version\n";
  my $corrected_header  = 0;
  my $corrected_version = 0;
  open(PRF, '+< PROBLEM-REPORT-FORM') or die "Opening $!";
  my $timestamp = $settings->{timestamp};
  my $header_line = "This is OpenDDS version $version, released $timestamp";
  my $version_line = "OpenDDS VERSION: $version";

  my $out = "";

  while (<PRF>) {
    if (s/^This is OpenDDS version .*, released.*$/$header_line/) {
      ++$corrected_header;
    } elsif (s/OpenDDS VERSION: .*$/$version_line/) {
      ++$corrected_version;
    }
    $out .= $_;
  }

  seek(PRF,0,0)                        or die "Seeking: $!";
  print PRF $out                       or die "Printing: $!";
  truncate(PRF,tell(PRF))              or die "Truncating: $!";
  close(PRF)                           or die "Closing: $!";

  return (($corrected_header == 1) && ($corrected_version == 1));
}
############################################################################

my @release_steps = (
  {
    title   => 'Verify git status is clean',
    skip    => 1,
    verify  => sub{verify_git_status_clean(@_)},
    message => sub{message_git_status_clean(@_)},
    # remedy  => sub{remedy_git_status_clean(@_)}
  },
  {
    title   => 'Update VERSION',
    skip    => 1,
    verify  => sub{verify_update_version_file(@_)},
    message => sub{message_update_version_file(@_)},
    remedy  => sub{remedy_update_version_file(@_)}
  },
  {
    title   => 'Add NEWS Section',
    skip    => 1,
    verify  => sub{verify_news_file_section(@_)},
    message => sub{message_news_file_section(@_)},
    remedy  => sub{remedy_news_file_section(@_)}
  },
  {
    title   => 'Update Version.h',
    skip    => 1,
    verify  => sub{verify_update_version_h_file(@_)},
    message => sub{message_update_version_h_file(@_)},
    remedy  => sub{remedy_update_version_h_file(@_)}
  },
  {
    title   => 'Update PROBLEM-REPORT-FORM',
    skip    => 1,
    verify  => sub{verify_update_prf_file(@_)},
    message => sub{message_update_prf_file(@_)},
    remedy  => sub{remedy_update_prf_file(@_)}
  },
  {
    title   => 'Update NEWS Section',
    skip    => 1,
    verify  => sub{verify_update_news_file(@_)},
    message => sub{message_update_news_file(@_)}
  },
  {
    title   => 'Verify remote arg',
    verify  => sub{verify_git_remote(@_)},
    message => sub{message_git_remote(@_)},
  },
  # Commit to git
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
  my @args = @ARGV[1..$#ARGV];
  foreach (@args) {
    if ($_ =~ /[0-9]+/) {
      return $_;
    }
  }
  return 0;
}

my %settings = (
  list      => any_arg_is("--list"),
  remedy    => any_arg_is("--remedy"),
  force     => any_arg_is("--force"),
  step      => numeric_arg(),
  version   => $ARGV[0],
  remote    => $ARGV[1] || "origin",
  timestamp => strftime("%a %b %e %T %Z %Y", @t),
  expected  => 'git@github.com:objectcomputing/OpenDDS.git'
);

my $half_divider = "-----------------------------------------";
my $divider = "$half_divider$half_divider";

sub run_step {
  my ($step_count, $step) = @_;
  # Output the title
  print "$step_count: $step->{title}\n";
  # Exit if we are just listing
  return if ($settings{list});
  # Run the verification
  if (!$step->{verify}(\%settings)) {
    # Failed
    print "$divider\n";
    print "  " . $step->{message}(\%settings) . "\n";

    # If we can remediate and --remedy set
    if ($settings{remedy} && $step->{remedy}) {
      if (!$step->{remedy}(\%settings)) {
        die "  !!!! Remediation did not complete\n";
      }
      # Reverify
      if (!$step->{verify}(\%settings)) {
        die "  !!!! Remediation did not pass verification\n";
      }
    } elsif ($settings{force} && $step->{skip}) {
      # Skip this step
    } elsif ($settings{force} && !$step->{skip}) {
      print "  Use --remedy to attempt a fix\n" if $step->{remedy};
      die "  Step can't be skipped";
    } elsif (!$settings{force} && $step->{skip}) {
      print "  Use --remedy to attempt a fix\n" if $step->{remedy};
      die "  Use --force to continue";
    } elsif (!$settings{force} && !$step->{skip}) {
      die;
    }
    print "$divider\n";
  }
}

sub validate_version_arg {
  my $version = $settings{version} || "";
  if ($version =~ /([0-9])+\.([0-9])+([0-9]+)?/) {
    $settings{major_version} = $1;
    $settings{minor_version} = $2;
    if ($3) {
      $settings{micro_version} = $3;
    } else {
      $settings{micro_version} = 0;
    }
    return 1;
  } else {
    return 0;
  }
}

if (validate_version_arg) {
  if (my $step_num = $settings{step}) {
    # Run one step
    run_step($step_num, $release_steps[$step_num - 1]);
  } else {
    my $step_count = 0;

    for my $step (@release_steps) {
      ++$step_count;
      run_step($step_count, $step);
    }
  }
} else {
  print usage();
}
