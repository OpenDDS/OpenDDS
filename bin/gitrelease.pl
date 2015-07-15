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
  return "$version" =~ /[0-9]+\.[0-9]+([0-9]+)?/
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
    if (/This is OpenDDS version $metaversion, released/ =~ $_) {
    }
  
    $correct = 1;
    last;
  }
  close(VERSION);

  return $correct;
}

sub message_update_version_file {
  return "VERSION file needs updating with current version"
}

sub remedy_update_version_file {
}

my @release_steps = (
  {
    title   => 'Verify version',
    verify  => sub{verify_version(@_)},
    message => sub{message_version(@_)},
  },
  {
    title   => 'Verify remote',
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
    title => 'Update NEWS'
  },
  {
    title => 'Update Version.h'
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
    print "$message\n";
    die;
  }
}

