#!/usr/bin/perl
use strict;
use warnings;
use Time::Piece;
use POSIX;
use Cwd;
use LWP::Simple;
use File::Basename;
use File::stat;
use lib dirname (__FILE__) . "/modules";
use ConvertFiles;
use Pithub::Repos::Releases;
use Data::Dumper;
use Net::FTP::File;

$ENV{TZ} = "UTC";
Time::Piece::_tzset;
my $timefmt = "%a %b %d %H:%M:%S %Z %Y";

sub usage {
  return "gitrelease.pl <version> [options]\n" .
         "    version:  release version in a.b or a.b.c notation\n" .
         "options:\n" .
         "  --list             just show step names (default perform check)\n" .
         "  --remedy           remediate problems where possible\n" .
         "  --remote=name      valid git remote for OpenDDS (default: origin)\n" .
         "  --branch=name      valid git branch for cloning (default: master)\n" .
         "  --github-user=name user or organization name for github updates (default: objectcomputing)\n" .
         "  --step=#           # of individual step to run (default: all)\n" .
         "  --force            keep going if possible (requires --step)\n" .
         "  --no-devguide      no devguide issued for this release\n"
}

sub normalizePath {
  my $val = shift;
  return Cwd::abs_path($val) if $val && -d $val && $val =~ /../;
  return $val;
}

sub email_announce_contents {
  my $settings = shift();
  my $devguide = "";
  if (!$settings->{nodevguide}) {
    $devguide =
      "An updated version of the OpenDDS Developer's Guide is available\n" .
      "from the same site in PDF format.\n";
  }

  my $result =
    "OpenDDS version $settings->{version} is now available for download.\n" .
    "Please see http://download.ociweb.com/OpenDDS for the download.\n\n" .
    $devguide . "\n" .
    "Updates in this OpenDDS version:\n\n";

  return $result;
}

sub news_contents_excerpt {
  return "";
}
############################################################################
sub parse_version {
  my $version = shift;
  my %result = ();
  if ($version =~ /([0-9])+\.([0-9])+\.?([0-9]+)?/) {
    $result{major} = $1;
    $result{minor} = $2;
    if ($3) {
      $result{micro} = $3;
    } else {
      $result{micro} = 0;
    }
  }
  return %result;
}

# Given a version string, return a numeric value for sorting purposes
sub version_to_value {
  my $tag_value = 0;
  my %result = parse_version(shift());
  if (%result) {
    $tag_value = (100.0 * ($result{major} || 0)) +
                          ($result{minor} || 0) +
                 ($result{micro} / 100.0);
  }
  return $tag_value;
}
############################################################################
sub verify_git_remote {
  my $result = 0;
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
  if ($url eq $settings->{git_url}) {
    $result = 1;
  } else {
    $settings->{alt_url} = $url;
  }
  return $result;
}

sub message_git_remote {
  my $settings = shift;
  my $remote = $settings->{remote};
  my $alt_url = $settings->{alt_url};
  my $start = "Remote $remote has url $alt_url, \n" .
         "which does not match expected URL $settings->{git_url}";
}

sub override_git_remote {
  my $settings = shift;
  my $remote = $settings->{remote};
  $settings->{git_url} = $settings->{alt_url};
  print "  Overriding remote $remote, url $settings->{alt_url}\n";
  # Reverify
  verify_git_remote($settings);
}

############################################################################
sub verify_git_status_clean {
  my ($settings, $strict) = @_;
  my $version = $settings->{version};
  my $clean = 1;
  my $status = open(GITSTATUS, 'git status -s|');
  my $modified = $settings->{modified};

  my $unclean = "";
  while (<GITSTATUS>) {
    if (/^...(.*)/) {
      # If this is not a known modified file, or if we are in strict mode
      if ($strict || !$modified->{$1}) {
        $unclean .= $_;
        $clean = 0;
      }
    }
  }
  close(GITSTATUS);

  $settings->{unclean} = $unclean;
  return $clean;
}

sub message_git_status_clean {
  my $settings = shift;
  return "The working directory is not clean:\n" . $settings->{unclean} .
         "  Commit to source control, or run git clean before continuing."
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
sub find_previous_tag {
  my $settings = shift();
  my $remote = $settings->{remote};
  my $version = $settings->{version};
  my $prev_version_tag = "";
  my $prev_version_value = 0;
  my $release_version_value = version_to_value($version);

  open(GITTAG, "git tag --list |") or die "Opening $!";
  while (<GITTAG>) {
    chomp;
    next unless /^DDS-([\d\.]*)/;
    my $tag_value = version_to_value($_);
    # If this is less than the release version, but the largest seen yet
    if (($tag_value < $release_version_value) &&
        ($tag_value > $prev_version_value)) {
      $prev_version_tag = $_;
      $prev_version_value = $tag_value;
    }
  }
  close(GITTAG);
  return $prev_version_tag;
}

sub verify_changelog {
  my $settings = shift();
  my $version = $settings->{version};
  my $status = open(CHANGELOG, $settings->{changelog});
  if ($status) {
    close(CHANGELOG);
  }
  return $status;
}

sub message_changelog {
  my $settings = shift();
  my $version = $settings->{version};
  return "File $settings->{changelog} missing";
}

sub format_comment_words {
  my $comment = shift;
  my $result = "";
  my @comment_words = split(/\s+/, $comment);
  # While there are words left to process...
  while (scalar(@comment_words) > 0) {
     # Start next line
     my $first_word = shift @comment_words;
     my $comment_line = "          $first_word";
     my $next = shift(@comment_words);
     while ($next &&
            (length($comment_line . " $next") <= 75)) {
       $comment_line .= " $next";
       $next = shift(@comment_words);
     }
     # Now next has been shifted, but doesn't fit on this line
     unshift(@comment_words, $next) if $next;
     $result .= "$comment_line\n";
  }
  return $result;
}

sub format_comment {
  my $comment = shift;
  my $result = "";
  my @comment_lines = split(/\n/, $comment);
  # While there are lines left to process...
  while (scalar(@comment_lines) > 0) {
    my $next_line = shift @comment_lines;
    if (!$next_line) {
      $result .= "\n";
    } elsif (length($next_line) < 65) {
      $result .= "          $next_line\n";
    } else {
      # Break next line into words
      my @comment_words = split(/\s+/, $next_line);
      while (scalar(@comment_words) > 0) {
        my $first_word = shift @comment_words;
        my $comment_line = "          $first_word";
        my $next = shift(@comment_words);
        # Fill up line
        while ($next &&
               (length($comment_line . " $next") <= 75)) {
          $comment_line .= " $next";
          $next = shift(@comment_words);
        }
        # Now next has been shifted, but doesn't fit on this line
        unshift(@comment_words, $next) if $next;
        $result .= "$comment_line\n";
      }
    }
  }
  return $result;
}

sub remedy_changelog {
  my $settings = shift();
  my $version = $settings->{version};
  my $remote = $settings->{remote};
  my $branch = $settings->{branch};
  my $prev_tag = find_previous_tag($settings);
  # Update so git log is correct
  open(GITREMOTE, "git remote update $remote|");
  while (<GITREMOTE>) {
  }
  close(GITREMOTE);
  my $author = 0;
  my $date = "";
  my $comment = "";
  my $commit = "";
  my $file_mod_list = "";
  my $changed = 0;

  print "  >> Creating $settings->{changelog} from git history\n";

  open(CHANGELOG, ">$settings->{changelog}") or die "Opening $!";

  open(GITLOG, "git log $prev_tag..$branch --name-status --date=raw |") or die "Opening $!";
  while (<GITLOG>) {
    chomp;
    if (/^commit /) {
      # print out previous
      if ($author) {
        print CHANGELOG $date . "  " .  $author . "\n";
        print CHANGELOG "        $commit\n";
        if ($file_mod_list) {
          print CHANGELOG "\n" . $file_mod_list;
        }
        print CHANGELOG "\n" . format_comment($comment) . "\n";
        $comment = "";
        $file_mod_list = "";
        $changed = 1;
      }
      $commit = $_;
    } elsif (/^Merge: *(.*)/) {
      # Ignore
    } elsif (/^Author: *(.*)/) {
      $author = $1;
      $author =~ s/</ </;
    } elsif (/^Date: *([0-9]+)/) {
      $date = POSIX::strftime($timefmt, gmtime($1));
    } elsif (/^ +(.*) */) {
      $comment .= "$1\n";
    } elsif (/^[AMD]\s+(.*) *$/) {
      $file_mod_list .= "        * $1:\n";
    } elsif (/^[CR][0-9]*\s+(.*) *$/) {
      $file_mod_list .= "        * $1:\n";
    }
  }
  # print out final
  if ($author) {
    print CHANGELOG $date . "  " .  $author . "\n";
    if ($file_mod_list) {
      print CHANGELOG "\n" . $file_mod_list;
    }
    print CHANGELOG "\n" . format_comment($comment) . "\n";
    $comment = "";
    $file_mod_list = "";
    $changed = 1;
    print CHANGELOG << "EOF";
Local Variables:
mode: change-log
add-log-time-format: (lambda () (progn (setq tz (getenv "TZ")) (set-time-zone-rule "UTC") (setq time (format-time-string "%a %b %e %H:%M:%S %Z %Y" (current-time))) (set-time-zone-rule tz) time))
indent-tabs-mode: nil
End:
EOF
  }
  close(GITLOG);
  close(CHANGELOG);

  return $changed;
}
############################################################################
sub verify_authors {
  my $status = 1;
  my $name = "";
  my $email = "";
  my $settings = shift();
  my %author_names;
  my %author_emails;
  my %author_email_aliases = (
    "jeffrey.j.schmitz\@gmail.com"               => "schmitzj\@ociweb.com",
    "jrw972\@users.noreply.github.com"           => "wilsonj\@ociweb.com",
    "brianjohnson5972\@users.noreply.github.com" => "johnsonb\@ociweb.com"
  );
  my %author_name_aliases = (
    "iamtheschmitzer" => "Jeff Schmitz",
    "johnsonb"        => "Brian Johnson",
  );
  open(AUTHORS, "AUTHORS") or die "Opening $!";
  while (<AUTHORS>) {
    chomp;
    ($name, $email) = split(/[<>]/);
    $name =~ s/^\s+|\s+$//g;
    $email =~ s/^\s+|\s+$//g;
    $author_names{$name} = 1;
    $author_emails{$email} = 1;
  }
  close (AUTHORS);

  open(CHANGELOG, $settings->{changelog}) or die "Opening $!";
  while (<CHANGELOG>) {
    if (/[0-9:]{8} GMT [0-9]{4} (.*) <(.*)>/) {
      $name = $1;
      $email = $2;
      $name =~ s/^\s+|\s+$//g;
      $email =~ s/^\s+|\s+$//g;
      $name = $author_name_aliases{$name} || $name;
      unless ($author_names{$name}) {
        print "Could not find name '$name' in AUTHORS file\n";
        $status = 0;
        # Add to hash so message goes away
        $author_names{$name} = 2;
      }
      $email = $author_email_aliases{$email} || $email;
      unless ($author_emails{$email}) {
        print "Could not find email '$email' in AUTHORS file\n";
        $status = 0;
        # Add to hash so message goes away
        $author_emails{$email} = 2;
      }
      next unless $status;
    }
  }
  return $status;
}

sub message_authors {
  my $settings = shift();
  return "AUTHORS file needs updating\n";
}

############################################################################
sub verify_news_file_section {
  my $settings = shift();
  my $version = $settings->{version};
  my $status = open(NEWS, 'NEWS.md');
  my $metaversion = quotemeta($version);
  my $has_version = 0;
  while (<NEWS>) {
    if ($_ =~ /Version $metaversion of OpenDDS/) {
      $has_version = 1;
    }
  }
  close(NEWS);

  return ($has_version);
}

sub message_news_file_section {
  my $settings = shift();
  my $version = $settings->{version};
  return "NEWS.md file release $version section needs updating";
}

sub remedy_news_file_section {
  my $settings = shift();
  my $version = $settings->{version};
  print "  >> Adding $version section to NEWS.md\n";
  print "  !! Manual update to NEWS.md needed\n";
  my $timestamp = $settings->{timestamp};
  my $outline = "This is OpenDDS version $version, released $timestamp";
  open(NEWS, "+< NEWS.md")                 or die "Opening: $!";
  my $out = "Version $version of OpenDDS\n" . <<"ENDOUT";
-------------------------------------------------------------------------------

##### Additions:
- TODO: Add your features here

##### Fixes:
- TODO: Add your fixes here

##### Notes:
- TODO: Add your notes here
_______________________________________________________________________________
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
  my $status = open(NEWS, 'NEWS.md');
  my $metaversion = quotemeta($version);
  my $has_version = 0;
  my $corrected_features = 1;
  my $corrected_fixes = 1;
  while (<NEWS>) {
    if ($_ =~ /Version $metaversion of OpenDDS/) {
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
  return "NEWS.md file needs updating with current version release notes";
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
sub message_commit_git_changes {
  my $settings = shift();
  return "The working directory is not clean:\n" . $settings->{unclean} .
         "Changed files must be committed to git.";
}

sub verify_git_changes_pushed {
  my $settings = shift();
  my $found = 0;
  my $target = "refs/tags/$settings->{git_tag}\$";
  open(GIT, "git ls-remote --tags |") or die "Opening $!";
  while (<GIT>) {
    chomp;
    if (/$target/) {
      $found = 1;
    }
  }
  close GIT;
  return $found;
}

sub message_git_changes_pushed {
  my $settings = shift();
  return "Changes and tags need to be pushed to $settings->{remote}\n";
}

sub remedy_git_changes_pushed {
  my $settings = shift();
  my $remote = $settings->{remote};
  print "pushing code\n";
  my $result = system("git push $remote $settings->{branch}");
  print "pushing tags with git push $remote --tags\n";
  $result = $result || system("git push $remote --tags");
  return !$result;
}

############################################################################
sub verify_git_tag {
  my $settings = shift();
  my $found = 0;
  open(GITTAG, "git tag --list 'DDS-*' |") or die "Opening $!";
  while (<GITTAG>) {
    chomp;
    if (/$settings->{git_tag}/) {
      $found = 1;
    }
  }
  close(GITTAG);
  return $found;
}

sub message_git_tag {
  my $settings = shift();
  return "Could not find a tag of $settings->{git_tag}.\n" .
         "Create annotated tag using\n" .
         "  >> git tag -a -m 'OpenDDS Release $settings->{version}'" .
         $settings->{git_tag};
}

sub remedy_git_tag {
  my $settings = shift();
  print "Creating tag $settings->{git_tag}\n";
  my $command = "git tag -a -m 'OpenDDS Release $settings->{version}' " .
                 $settings->{git_tag};
  my $result = system($command);
  return !$result;
}
############################################################################
sub verify_clone_tag {
  my $settings = shift();
  my $correct = 0;
  if (-d $settings->{clone_dir}) {
    my $curdir = getcwd;
    chdir $settings->{clone_dir};
    # Using git describe for compatibility with older versions of git
    open(GIT_DESCRIBE, "git describe --tags |") or die "git describe --tags $!";
    while (<GIT_DESCRIBE>) {
      if (/^$settings->{git_tag}/) {
        $correct = 1;
        next;
      }
    }
    close GIT_DESCRIBE;
    chdir $curdir;
  }
  return $correct;
}

sub message_clone_tag {
  my $settings = shift();
  if (-d $settings->{clone_dir}) {
    return "Directory $settings->{clone_dir} did not clone tag $settings->{git_tag}\n";
  } else {
    return "Could not see directory $settings->{clone_dir}\n";
  }
}

sub remedy_clone_tag {
  my $settings = shift();
  my $result = 0;
  if (!-d $settings->{clone_dir}) {
    print "Cloning url $settings->{git_url}\ninto $settings->{clone_dir}\n";
    $result = system("git clone $settings->{git_url} $settings->{clone_dir}");
  }
  if (!$result) {
    my $curdir = getcwd;
    chdir($settings->{clone_dir});
    print "Checking out tag $settings->{git_tag}\n";
    $result = system("git checkout $settings->{git_tag}");
    chdir($curdir);
  }
  return !$result;
}
############################################################################
sub verify_move_changelog {
  my $settings = shift();
  my $target = $settings->{clone_dir} . "/ChangeLog";
  return -f $settings->{clone_dir} . "/ChangeLog";
}

sub message_move_changelog {
  my $settings = shift();
  my $src = $settings->{clone_dir} . "/" . $settings->{changelog};
  my $target = $settings->{clone_dir} . "/ChangeLog";
  return "ChangeLog must be moved from $src to $target";
}

sub remedy_move_changelog {
  my $settings = shift();
  my $src = $settings->{clone_dir} . "/" . $settings->{changelog};
  my $target = $settings->{clone_dir} . "/ChangeLog";
  rename($src, $target);
  print "Changelog moved\n";
  return 1;
}

############################################################################
sub verify_tgz_source {
  my $settings = shift();
  my $file = join("/", $settings->{parent_dir}, $settings->{tgz_src});
  my $good = 0;
  if (-f $file) {
    # Check if it is in the right format
    my $basename = basename($settings->{clone_dir});
    open(TGZ, "gzip -c -d $file | tar -tvf - |") or die "Opening $!";
    my $target = join("/", $basename, 'VERSION');
    while (<TGZ>) {
      if (/$target/) {
        $good = 1;
        last;
      }
    }
    close TGZ;
  }
  return $good;
}

sub message_tgz_source {
  my $settings = shift();
  my $file = join("/", $settings->{parent_dir}, $settings->{tgz_src});
  if (!-f $file) {
    return "Could not find file $file";
  } else {
    return "File $file is not in the right format";
  }
}

sub remedy_tgz_source {
  my $settings = shift();
  my $file = join("/", $settings->{parent_dir}, $settings->{tgz_src});
  my $curdir = getcwd;
  chdir($settings->{parent_dir});
  print "Creating file $settings->{tar_src}\n";
  my $basename = basename($settings->{clone_dir});
  # --exclude-vcs nor available in some tar versions
  print "Removing git-specific directories\n";
  my $result = system("find . -name '.git*' | xargs rm -rf");
  $result = $result || system("tar -cf $settings->{tar_src} $basename");
  if (!$result) {
    print "Gzipping file $settings->{tar_src}\n";
    $result = system("gzip $settings->{tar_src}");
  }
  chdir($curdir);
  return !$result;
}
############################################################################
sub verify_zip_source {
  my $settings = shift();
  my $file = join("/", $settings->{parent_dir}, $settings->{zip_src});
  return (-f $file);
}

sub message_zip_source {
  my $settings = shift();
  my $file = join("/", $settings->{parent_dir}, $settings->{zip_src});
  return "Could not find file $file";
}

sub remedy_zip_source {
  my $settings = shift();
  my $file = join("/", $settings->{parent_dir}, $settings->{zip_src});
  my $curdir = getcwd;
  chdir($settings->{clone_dir});
  # zip -x .git .gitignore does not exclude as advertised
  print "Removing git-specific directories\n";
  my $result = system("find . -name '.git*' | xargs rm -rf");
  # convert line endings
  print "Converting source files to Windows line endings\n";
  my $converter = new ConvertFiles();
  my ($stat, $error) = $converter->convert(".");
  if (!$stat) {
    print $error;
    return 0;
  }

  if (!$result) {
    print "Creating file $settings->{zip_src}\n";
    $result = system("zip ../$settings->{zip_src} -qq -r . -x '.git*'");
  }
  chdir($curdir);
  return !$result;
}
############################################################################
sub verify_md5_checksum{
  my $settings = shift();
  my $file = join("/", $settings->{parent_dir}, $settings->{md5_src});
  return (-f $file);
}

sub message_md5_checksum{
  return "You need to generate the MD5 checksum file";
}

sub remedy_md5_checksum{
  my $settings = shift();
  print "Creating file $settings-{md5_src}\n";
  my $md5_file = join("/", $settings->{parent_dir}, $settings->{md5_src});
  my $tgz_file = join("/", $settings->{parent_dir}, $settings->{tgz_src});
  my $zip_file = join("/", $settings->{parent_dir}, $settings->{zip_src});
  return !system("md5sum $tgz_file $zip_file > $md5_file");
}
############################################################################
sub verify_gen_doxygen {
  my $settings = shift();
  return (-f "$settings->{clone_dir}/html/dds/index.html");
}

sub message_gen_doxygen {
  my $settings = shift();
  return "Doxygen documentation needs generating in dir $settings->{clone_dir}\n";
}

sub remedy_gen_doxygen {
  my $settings = shift();
  my $generated = 0;
  if ($ENV{ACE_ROOT}) {
    $ENV{ACE_ROOT} = normalizePath($ENV{ACE_ROOT});
    my $curdir = getcwd;
    chdir($settings->{clone_dir});
    $ENV{DDS_ROOT} = getcwd;
    my $result = system("$ENV{ACE_ROOT}/bin/generate_doxygen.pl -exclude_ace -include_dds -is_release");
    chdir($curdir);
    if (!$result) {
      $generated = 1;
    }
  } else {
    print "ACE_ROOT is not set\n";
  }
  return $generated;
}
############################################################################
sub verify_tgz_doxygen {
  my $settings = shift();
  my $file = join("/", $settings->{parent_dir}, $settings->{tgz_dox});
  my $good = 0;
  if (-f $file) {
    open(TGZ, "gzip -c -d $file | tar -tvf - |") or die "Opening $!";
    my $target = "html/dds/index.html";
    while (<TGZ>) {
      if (/$target/) {
        $good = 1;
        last;
      }
    }
    close TGZ;
  }
  return $good;
}

sub message_tgz_doxygen {
  my $settings = shift();
  my $file = join("/", $settings->{parent_dir}, $settings->{tgz_dox});
  return "Could not find file $file";
}

sub remedy_tgz_doxygen {
  my $settings = shift();
  my $file = join("/", $settings->{parent_dir}, $settings->{tar_dox});
  my $curdir = getcwd;
  chdir($settings->{clone_dir});
  print "Creating file $settings->{tar_dox}\n";
  my $result = system("tar -cf ../$settings->{tar_dox} html");
  if (!$result) {
    print "Gzipping file $settings->{tar_dox}\n";
    $result = system("gzip ../$settings->{tar_dox}");
  }
  chdir($curdir);
  return !$result;
}
############################################################################
sub verify_zip_doxygen {
  my $settings = shift();
  my $file = join("/", $settings->{parent_dir}, $settings->{zip_dox});
  return (-f $file);
}

sub message_zip_doxygen {
  my $settings = shift();
  my $file = join("/", $settings->{parent_dir}, $settings->{zip_dox});
  return "Could not find file $file";
}

sub remedy_zip_doxygen {
  my $settings = shift();
  my $curdir = getcwd;
  print "Converting doxygen files to Windows line endings\n";
  my $converter = new ConvertFiles();
  my ($stat, $error) = $converter->convert($settings->{clone_dir} . "/html");
  if (!$stat) {
    print $error;
    return 0;
  }

  chdir("$settings->{clone_dir}/html");
  my $file = "../../$settings->{zip_dox}";
  print "Creating file $settings->{zip_dox}\n";
  my $result = system("zip $file -qq -r dds");
  chdir($curdir);
  return !$result;
}
############################################################################
sub verify_devguide {
  my $settings = shift();
  if (!$settings->{nodevguide}) {
    return (-f "$settings->{parent_dir}/$settings->{devguide}" &&
            -f "$settings->{parent_dir}/OpenDDS-latest.pdf");
  } else {
    return 1;
  }
}

sub message_devguide {
  my $settings = shift();
  return "Devguide missing";
}

sub remedy_devguide {
  my $settings = shift();
  if (!$settings->{nodevguide}) {
    my $devguide_url = "svn+ssh://svn.ociweb.com/devguide/opendds/trunk/$settings->{devguide}";
    print "Downloading devguide\n$devguide_url\n";
    my $result = system("svn cat $devguide_url > $settings->{parent_dir}/$settings->{devguide}");
    $result = $result || system("svn cat $devguide_url > $settings->{parent_dir}/OpenDDS-latest.pdf");
    return !$result;
  } else {
    return 1;
  }
}

############################################################################
sub verify_ftp_upload {
  my $settings = shift();
  my $url = "http://download.ociweb.com/OpenDDS/";
  my $content = get($url);

  my @files = (
      "$settings->{base_name}-doxygen.tar.gz",
      "$settings->{base_name}-doxygen.zip",
      "$settings->{base_name}.tar.gz",
      "$settings->{base_name}.zip",
      "$settings->{base_name}.md5",
      "OpenDDS-latest.pdf"
  );
  if (!$settings->{nodevguide}) {
    push(@files , "$settings->{base_name}.pdf");
  }
  foreach my $file (@files) {
    if ($content =~ /$file/) {
    } else {
      print "$file not found at $url\n";
      return 0;
    }
  }
  return 1;
}

sub message_ftp_upload {
  return "Release needs to be uploaded to ftp site";
}

sub remedy_ftp_upload {
  my $settings = shift();
  my $version = $settings->{version};
  my $FTP_DIR = "downloads/OpenDDS";
  my $PRIOR_RELEASE_PATH = 'previous-releases/';

  # login to ftp server and setup binary file transfers
  my $ftp = Net::FTP->new($settings->{ftp_host}, Debug => 0, Port => $settings->{ftp_port})
      or die "Cannot connect to $settings->{ftp_host}: $@";

  $ftp->login($settings->{ftp_user}, $settings->{ftp_password})
      or die "Cannot login ", $ftp->message;

  $ftp->cwd($FTP_DIR)
      or die "Cannot change dir to $FTP_DIR ", $ftp->message;

  my @current_release_files = $ftp->ls()
      or die "Cannot ls() $FTP_DIR ", $ftp->message;

  $ftp->binary;

  my @new_release_files = (
      "$settings->{base_name}-doxygen.tar.gz",
      "$settings->{base_name}-doxygen.zip",
      "$settings->{base_name}.tar.gz",
      "$settings->{base_name}.zip",
      "$settings->{base_name}.md5",
      "OpenDDS-latest.pdf"
  );
  if (!$settings->{nodevguide}) {
    push(@new_release_files , "$settings->{base_name}.pdf");
  }

  my %release_file_map = map { $_ => 1 } @new_release_files;

  # handle previous release files
  foreach my $file (@current_release_files) {

    # skip directories, this is not the file you're looking for...
    my $isDir = $ftp->isdir($file);
    next if $isDir;

    # replace 'OpenDDS-latest.pdf'
    if ($file eq 'OpenDDS-latest.pdf') {
      print "deleting $file\n";
      $ftp->delete($file);

      my $local_file = join("/", $settings->{parent_dir}, $file);
      print "uploading $file\n";
      $ftp->put($local_file, $file);
    }

    # transfer files to prior release path that are not in the current release set
    unless ( exists($release_file_map{$file})){
      my $new_path = $PRIOR_RELEASE_PATH . $file;
      print "moving $file to $new_path\n";
      $ftp->rename($file,$new_path);
    }
  }

  # upload new release files
  foreach my $file (@new_release_files) {
    print "uploading $file\n";
    my $local_file = join("/", $settings->{parent_dir}, $file);
    $ftp->put($local_file, $file);
  }

  $ftp->quit;
  return 1;
}

############################################################################
sub verify_github_upload {
  my $settings = shift();
  my $verified = 0;

  my $r = Pithub::Repos::Releases->new;
  my $release_list = $r->list(
      user => $settings->{github_user},
      repo => $settings->{github_repo},
  );
  unless ( $release_list->success ) {
    printf "error accessing github: %s\n", $release_list->response->status_line;
  } else {
    while ( my $row = $release_list->next ) {
      if ($row->{tag_name} eq $settings->{git_tag}){
        #printf "%d\t[%s]\n",$row->{id},$row->{tag_name};
        $verified = 1;
        last;
      }
    }
  }
  return $verified;
}

sub message_github_upload {
  return "Release needs to be uploaded to github site";
}

sub remedy_github_upload {
  my $settings = shift();

  my $rc = 1;

  # printf "%s:%s\n%s\n", $settings->{github_user},$settings->{github_repo},$settings->{github_token};

  my $ph = Pithub::Repos::Releases->new(
      user => $settings->{github_user},
      repo => $settings->{github_repo},
      token => $settings->{github_token}
  );
  my @lines;
  open(my $news, "NEWS.md") or die "Can't read NEWS.md file";
  while (<$news>) {
    if (/^_____/) {
      last;
    }
    push (@lines, $_);
  }
  close $news;
  my $header  = "#### Release notes for Version $settings->{version} of OpenDDS\n";
  my $middle = join("",@lines);
  my $footer = "#### Using the GitHub \"releases\" page\nDownload $settings->{zip_src} (Windows) or $settings->{tgz_src} (Linux/Solaris/MacOSX) instead of using the GitHub-generated \"source code\" links.";
  my $text = $header.$middle.$footer;
  my $release = $ph->create(
      data => {
          name => 'OpenDDS ' . $settings->{version},
          tag_name => 'DDS-' . $settings->{version},
          body => $text
      }
  );
  unless ( $release->success ) {
    printf "error accessing github: %s\n", $release->response->status_line;
    $rc = 0;
  } else {
    for my $f ("$settings->{parent_dir}/$settings->{tgz_src}",
               "$settings->{parent_dir}/$settings->{zip_src}") {
      open(my $fh, $f) or die "Can't open";
      binmode $fh;
      my $size = stat($fh)->size;
      my $data;
      read $fh, $data, $size or die "Can't read";
      my $mime = ($f =~ /\.gz$/) ? 'application/gzip' : 'application/x-zip-compressed';
      my $asset = $ph->assets->create(
          release_id => $release->content->{id},
          name => $f,
          content_type => $mime,
          data => $data
      );
      unless ( $asset->success ) {
        printf "error accessing github: %s\n", $asset->response->status_line;
        $rc = 0;
      }
    }
  }
  return $rc;
}

############################################################################
sub verify_website_release {
  # verify there are no differences between website-next-release branch and gh-pages branch
  my $status = open(GITDIFF, 'git diff origin/website-next-release origin/gh-pages|');
  my $delta = "";
  while (<GITDIFF>) {
    if (/^...(.*)/) {
      $delta .= $_;
    }
  }
  close(GITDIFF);
  # return 1 to pass
  # return 0 to fail
  return (length($delta) == 0) ? 1 : 0;
}

sub message_website_release {
  return 'origin/website-next-release branch needs to merge into origin/gh-pages branch';
}

sub remedy_website_release {
  my $settings = shift();
  print "Releasing gh-pages website\n";
  my $rc = 0;
#  $rc = system('git checkout gh-pages');
#  if (!$rc) {
#    $rc = system('git merge website-next-release');
#  }
#  if (!$rc) {
#    $rc =   system ('git push');
#  }
  return !$rc;
}

############################################################################
sub verify_email_list {
  # Can't verify
}

sub message_email_dds_release_announce {
  return 'Email needs to be sent to dds-release-announce@ociweb.com';
}

sub remedy_email_dds_release_announce {
  my $settings = shift;
  return 'Email this text to dds-release-announce@ociweb.com' . "\n\n" .

  "Approved: postthisrelease\n\n" .
  email_announce_contents($settings) .
  news_contents_excerpt($settings);
}

############################################################################
sub verify_news_template_file_section {
  my $settings = shift();
  my $status = open(NEWS, 'NEWS.md');
  my $has_news_template = 0;
  while (<NEWS>) {
    if ($_ =~ /Version X.Y of OpenDDS\./) {
      $has_news_template = 1;
    }
  }
  close(NEWS);

  return ($has_news_template);
}

sub message_news_template_file_section {
  my $settings = shift();
  # my $version = $settings->{version};
  return "next NEWS.md file release X.Y section missing";
}

sub remedy_news_template_file_section {
  my $settings = shift();
  print "  >> Adding next version template section to NEWS.md\n";
  print "  !! Manual update to NEWS.md needed\n";
  open(NEWS, "+< NEWS.md") or die "Opening: $!";
  my $out = "Version X.Y of OpenDDS.\n" . <<"ENDOUT";
-------------------------------------------------------------------------------

##### Additions:
- TODO: Add your features here

##### Fixes:
- TODO: Add your fixes here

##### Notes:
- TODO: Add your notes here
_______________________________________________________________________________
ENDOUT

  $out .= join("", <NEWS>);
  seek(NEWS,0,0)            or die "Seeking: $!";
  print NEWS $out           or die "Printing: $!";
  truncate(NEWS,tell(NEWS)) or die "Truncating: $!";
  close(NEWS)               or die "Closing: $!";
  return 1;
}

############################################################################
sub ignore_step {
  my $step = shift;
  print "  Ignoring step $step\n";
  return 1;
}

############################################################################
my %steps_run = ();

my %release_step_hash  = (
  'Update VERSION' => {
    verify  => sub{verify_update_version_file(@_)},
    message => sub{message_update_version_file(@_)},
    remedy  => sub{remedy_update_version_file(@_)},
    force   => sub{ignore_step("Update VERSION")}
  },
  'Update Version.h' => {
    verify  => sub{verify_update_version_h_file(@_)},
    message => sub{message_update_version_h_file(@_)},
    remedy  => sub{remedy_update_version_h_file(@_)},
    force   => sub{ignore_step("Update Version.h")}
  },
  'Update PROBLEM-REPORT-FORM' => {
    verify  => sub{verify_update_prf_file(@_)},
    message => sub{message_update_prf_file(@_)},
    remedy  => sub{remedy_update_prf_file(@_)},
    force   => sub{ignore_step('Update PROBLEM-REPORT-FORM')}
  },
  'Verify remote arg' => {
    verify  => sub{verify_git_remote(@_)},
    message => sub{message_git_remote(@_)},
    remedy  => sub{override_git_remote(@_)},
  },
  'Create ChangeLog' => {
    verify  => sub{verify_changelog(@_)},
    message => sub{message_changelog(@_)},
    remedy  => sub{remedy_changelog(@_)},
    force   => sub{ignore_step('Create ChangeLog')}
  },
  'Update Authors' => {
    verify  => sub{verify_authors(@_)},
    message => sub{message_authors(@_)},
    remedy  => sub{remedy_authors(@_)},
    force   => sub{ignore_step('Update Authors')}
  },
  'Add NEWS Section' => {
    verify  => sub{verify_news_file_section(@_)},
    message => sub{message_news_file_section(@_)},
    remedy  => sub{remedy_news_file_section(@_)},
    force   => sub{ignore_step('Add NEWS Section')}
  },
  'Update NEWS Section' => {
    verify  => sub{verify_update_news_file(@_)},
    message => sub{message_update_news_file(@_)},
    force   => sub{ignore_step('Update NEWS Section')}
  },
  'Commit changes to GIT' => {
    verify  => sub{verify_git_status_clean(@_, 1)},
    message => sub{message_commit_git_changes(@_)}
  },
  'Verify changes pushed' => {
    prereqs => ('Verify remote arg'),
    verify  => sub{verify_git_changes_pushed(@_, 1)},
    message => sub{message_git_changes_pushed(@_)},
    remedy  => sub{remedy_git_changes_pushed(@_)}
  },
  'Create git tag' => {
    verify  => sub{verify_git_tag(@_)},
    message => sub{message_git_tag(@_)},
    remedy  => sub{remedy_git_tag(@_)}
  },
  'Clone tag' => {
    prereqs => ('Verify remote arg'),
    verify  => sub{verify_clone_tag(@_)},
    message => sub{message_clone_tag(@_)},
    remedy  => sub{remedy_clone_tag(@_)}
  },
  'Move changelog' => {
    verify  => sub{verify_move_changelog(@_)},
    message => sub{message_move_changelog(@_)},
    remedy  => sub{remedy_move_changelog(@_)}
  },
  'Create unix release archive' => {
    verify  => sub{verify_tgz_source(@_)},
    message => sub{message_tgz_source(@_)},
    remedy  => sub{remedy_tgz_source(@_)}
  },
  'Create windows release archive' => {
    verify  => sub{verify_zip_source(@_)},
    message => sub{message_zip_source(@_)},
    remedy  => sub{remedy_zip_source(@_)}
  },
  'Generate doxygen' => {
    verify  => sub{verify_gen_doxygen(@_)},
    message => sub{message_gen_doxygen(@_)},
    remedy  => sub{remedy_gen_doxygen(@_)} # $ACE_ROOT/bin/generate_doxygen.pl
  },
  'Create unix doxygen archive' => {
    verify  => sub{verify_tgz_doxygen(@_)},
    message => sub{message_tgz_doxygen(@_)},
    remedy  => sub{remedy_tgz_doxygen(@_)}
  },
  'Create windows doxygen archive' => {
    verify  => sub{verify_zip_doxygen(@_)},
    message => sub{message_zip_doxygen(@_)},
    remedy  => sub{remedy_zip_doxygen(@_)}
  },
  'Create md5 checksum' => {
    verify  => sub{verify_md5_checksum(@_)},
    message => sub{message_md5_checksum(@_)},
    remedy  => sub{remedy_md5_checksum(@_)}
  },
  'Download Devguide' => {
    verify  => sub{verify_devguide(@_)},
    message => sub{message_devguide(@_)},
    remedy  => sub{remedy_devguide(@_)},
  },
  'Upload to FTP Site' => {
    verify  => sub{verify_ftp_upload(@_)},
    message => sub{message_ftp_upload(@_)},
    remedy => sub{remedy_ftp_upload(@_)}
  },
  'Upload to GitHub' => {
     verify  => sub{verify_github_upload(@_)},
     message => sub{message_github_upload(@_)},
     remedy  => sub{remedy_github_upload(@_)}
  },
  'Release Website' => {
    verify  => sub{verify_website_release(@_)},
    message => sub{message_website_release(@_)},
    remedy  => sub{remedy_website_release(@_)},
  },
  'Email DDS-Release-Announce list' => {
    verify  => sub{verify_email_list(@_)},
    message => sub{message_email_dds_release_announce(@_)},
    message => sub{remedy_email_dds_release_announce(@_)}
  },
 'Add NEWS Template Section' => {
    verify  => sub{verify_news_template_file_section(@_)},
    message => sub{message_news_template_file_section(@_)},
    remedy  => sub{remedy_news_template_file_section(@_)},
  },
  'Commit NEWS Template Section' => {
    verify  => sub{verify_git_status_clean(@_, 1)},
    message => sub{message_commit_git_changes(@_)}
  }

);

my @ordered_steps = (
  'Update VERSION',
  'Update Version.h',
  'Update PROBLEM-REPORT-FORM',
  'Verify remote arg',
  'Create ChangeLog',
  'Update Authors',
  'Add NEWS Section',
  'Update NEWS Section',
  'Commit changes to GIT',
  'Create git tag',
  'Verify changes pushed',
  'Clone tag',
  'Move changelog',
  'Create unix release archive',
  'Create windows release archive',
  'Generate doxygen',
  'Create unix doxygen archive',
  'Create windows doxygen archive',
  'Create md5 checksum',
  'Download Devguide',
  'Upload to FTP Site',
  'Upload to GitHub',
  'Release Website',
  'Email DDS-Release-Announce list',
  'Add NEWS Template Section',
  'Commit NEWS Template Section',
);

sub any_arg_is {
  my $match = shift;
  foreach my $arg (@ARGV) {
    if ($arg eq $match) {
      return 1;
    }
  }
  return 0;
}

sub numeric_arg_value {
  my $name = shift;
  my @args = @ARGV[1..$#ARGV];
  my $arg_str = join(" ", @args);
  if ($arg_str =~ /$name ?=? ?([0-9]+)/) {
    return $1;
  }
}

sub string_arg_value {
  my $name = shift;
  my @args = @ARGV[1..$#ARGV];
  my $arg_str = join(" ", @args);
  if ($arg_str =~ /$name ?=? ?([^ ]+)/) {
    return $1;
  }
}

my $version = $ARGV[0] || "";

my %base_settings = (
  base_name   => "OpenDDS-$version",
  parent_dir  => '../OpenDDS-Release',
  github_user => string_arg_value("--github-user") || "objectcomputing",
);

my %settings = (
    list         => any_arg_is("--list"),
    remedy       => any_arg_is("--remedy"),
    force        => any_arg_is("--force"),
    nodevguide   => any_arg_is("--no-devguide"),
    step         => numeric_arg_value("--step"),
    remote       => string_arg_value("--remote") || "origin",
    branch       => string_arg_value("--branch") || "master",
    github_user  => $base_settings{github_user},
    version      => $version,
    base_name    => $base_settings{base_name},
    git_tag      => "DDS-$version",
    parent_dir   => $base_settings{parent_dir},
    clone_dir    => join("/", $base_settings{parent_dir}, $base_settings{base_name}),
    tar_src      => "$base_settings{base_name}.tar",
    tgz_src      => "$base_settings{base_name}.tar.gz",
    zip_src      => "$base_settings{base_name}.zip",
    md5_src      => "$base_settings{base_name}.md5",
    tar_dox      => "$base_settings{base_name}-doxygen.tar",
    tgz_dox      => "$base_settings{base_name}-doxygen.tar.gz",
    zip_dox      => "$base_settings{base_name}-doxygen.zip",
    devguide     => "$base_settings{base_name}.pdf",
    timestamp    => POSIX::strftime($timefmt, gmtime),
    git_url      => "git\@github.com:$base_settings{github_user}/OpenDDS.git",
    github_repo  => 'OpenDDS',
    github_token => $ENV{GITHUB_TOKEN},
    ftp_user     => $ENV{FTP_USERNAME},
    ftp_password => $ENV{FTP_PASSWD},
    ftp_host     => $ENV{FTP_HOST},
    changelog    => "docs/history/ChangeLog-$version",
    modified     => {"NEWS.md" => 1,
        "PROBLEM-REPORT-FORM" => 1,
        "VERSION" => 1,
        "dds/Version.h" => 1,
        "docs/history/ChangeLog-$version" => 1,
        "tools/scripts/gitrelease.pl" => 1}
);

my $half_divider = '-' x 40;
my $divider = $half_divider x 2;

sub run_step {
  my ($settings, $step_count, $title, $release_step_hash, $steps_run) = @_;
  my $step = $release_step_hash->{$title};

  if (!$settings->{list}) {
    if ($step->{prereqs}) {
      for my $prereq ($step->{prereqs}) {
        unless ($steps_run->{$prereq}) {
          run_step($settings, 'PREREQ', $prereq, $release_step_hash, $steps_run);
        }
      }
    }
  }

  # Output the title
  print "$step_count: $title\n";
  # Exit if we are just listing
  return if ($settings{list});
  # Run the verification
  if (!$step->{verify}(\%settings)) {
    # Failed
    print "$divider\n";
    print "  " . $step->{message}(\%settings) . "\n";

    my $remedied = 0;
    my $forced  = 0;
    # If a remedy is available and --remedy specified
    if ($step->{remedy} && $settings{remedy}) {
      # Try remediation
      if (!$step->{remedy}(\%settings)) {
        print "  !!!! Remediation did not complete\n";
      # Reverify
      } elsif (!$step->{verify}(\%settings)) {
        print "  !!!! Remediation did not pass verification\n";
      } else {
        $remedied = 1;
      }
    }

    # If not remedied, try forcing
    if (!$remedied && $step->{force} && $settings{force}) {
      if ($step->{force}(\%settings)) {
        $forced = 1;
      } else {
        print "  !!!! Forcing did not complete\n";
      }
    }

    if (!($remedied || $forced)) {
      if ($step->{remedy} && !$settings{remedy}) {
        print "  Use --remedy to attempt a fix\n";
      }
      if ($step->{force} && !$settings{force}) {
        print "  Use --force to force past this step\n";
      }
    }

    die unless ($remedied || $forced);
    $steps_run->{$title} = 1;
    print "$divider\n";
  }
}

sub validate_version_arg {
  my $version = $settings{version} || "";
  my %result = parse_version($version);
  if (%result) {
    $settings{major_version} = $result{major};
    $settings{minor_version} = $result{minor};
    $settings{micro_version} = $result{micro};
    return 1;
  } elsif ($settings{list}) {
    return 1;
  } else {
    return 0;
  }
}

if ($settings{list} || validate_version_arg()) {
  if (my $step_num = $settings{step}) {
    # Run one step
    my $step_name = $ordered_steps[$step_num - 1];
    run_step(\%settings,
             $step_num, $step_name, \%release_step_hash, \%steps_run);
  } else {
    my $step_count = 0;

    for my $step_name (@ordered_steps) {
      ++$step_count;
      run_step(\%settings,
               $step_count, $step_name, \%release_step_hash, \%steps_run);
    }
  }
} else {
  print usage();
}
