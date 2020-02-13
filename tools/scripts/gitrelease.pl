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
use File::Temp qw/ tempdir /;
use File::Path qw(make_path);

my $base_name_prefix = "OpenDDS-";
my $default_github_user = "objectcomputing";
my $default_remote = "origin";
my $default_branch = "master";
my $release_branch_prefix = "branch-";
my $repo_name = "OpenDDS";
my $default_download_url = "http://download.ociweb.com/OpenDDS";
my $ace_tao_filename = "ACE+TAO-2.2a_with_latest_patches_NO_makefiles.tar.gz";
my $ace_tao_url = "http://download.ociweb.com/TAO-2.2a/$ace_tao_filename";
my $ace_root = "ACE_wrappers";
my $git_name_prefix = "DDS-";

$ENV{TZ} = "UTC";
Time::Piece::_tzset;
my $timefmt = "%a %b %d %H:%M:%S %Z %Y";

my $timestamp_marker = "%TIMESTAMP%";

# Number of lines after start of file to insert news_template
my $news_header_lines = 2;

my $news_template = "## Version %s of OpenDDS\n" . <<"ENDOUT";
Released %s

### Additions:
- TODO: Add your features here

### Fixes:
- TODO: Add your fixes here

### Notes:
- TODO: Add your notes here

ENDOUT

my $insert_news_template_after_line = 1;
sub insert_news_template($$) {
  my $version = shift;
  my $timestamp = shift;

  # Read News File
  open my $news_file, '<', "NEWS.md";
  my @lines = <$news_file>;
  close $news_file;

  # Insert Template
  my @new_lines = ();
  push(@new_lines, @lines[0..$insert_news_template_after_line]);
  push(@new_lines, sprintf($news_template, $version, $timestamp));
  push(@new_lines, @lines[$insert_news_template_after_line+1..scalar(@lines)-1]);

  # Write News File with Template
  open $news_file, '>', "NEWS.md";
  foreach my $line (@new_lines) {
    print $news_file $line;
  }
  close $news_file;
}

sub usage {
  return "gitrelease.pl WORKSPACE VERSION [STEPS] [options]\n" .
         "gitrelease.pl --list | --help | -h\n" .
         "\n" .
         "Positional Arguments:\n" .
         "    WORKSPACE:       Path of intermediate files directory. If it\n" .
         "                     doesn't exist, it will be created.\n" .
         "                     This should a new one for each release.\n" .
         "    VERSION:         release version in a.b or a.b.c notation\n" .
         "    STEPS:           Optional, Steps to perform, default is all\n" .
         "                     See \"Step Expressions\" Below for what it accepts.\n" .
         "\n" .
         "Options:\n" .
         "  --help | -h        Print this message\n" .
         "  --list             just show step names (default perform check)\n" .
         "  --remedy           remediate problems where possible\n" .
         "  --force            keep going if possible\n" .
         "  --remote=name      valid git remote for OpenDDS (default: ${default_remote})\n" .
         "  --branch=name      valid git branch for cloning (default: ${default_branch})\n" .
         "  --github-user=name user or organization name for github updates (default: ${default_github_user})\n" .
         "  --download-url=url URL to verify FTP uploads (default: ${default_download_url}\n" .
         "  --micro            Do a patch/micro level release\n" .
         "                     The difference is we skip the following:\n" .
         "                         devguide, doxygen, website, release branch, and adding NEWS Template\n" .
         "                     Requires --branch\n" .
         "  --skip-devguide    Skip getting and including the devguide\n" .
         "  --skip-doxygen     Skip getting ACE/TAO and generating and including the doxygen docs\n" .
         "  --skip-website     Skip updating the website\n" .
         "  --skip-ftp         Skip the FTP upload\n" .
         "\n" .
         "Environment Variables:\n" .
         "  GITHUB_TOKEN       GitHub token with repo access to publish release on GitHub.\n" .
         "  FTP_USERNAME       FTP Username\n" .
         "  FTP_PASSWD         FTP Password\n" .
         "  FTP_HOST           FTP Server Address\n" .
         "\n" .
         "Step Expressions\n" .
         "  The STEPS argument accepts a specific notation for what steps to run.\n" .
         "  Later subexpressions override earlier ones and the initial list of\n" .
         "  steps the expression is modifying is empty at the beginning unless the\n" .
         "  first subexpression is negative, then it starts with all steps.\n" .
         "  Here are some examples:\n" .
         "    5\n" .
         "      Just do step 5\n" .
         "    3-7\n" .
         "      Do steps 3, 4, 5, 6, and 7\n" .
         "    -8\n" .
         "      Do steps up to and including step 8 and stop\n" .
         "    8-\n" .
         "      Do step 8 and all the steps after that\n" .
         "    ^5\n" .
         "      Do all steps except 5\n" .
         "    ^5-7\n" .
         "      Do all steps except 5, 6, and 7\n" .
         "    Finally you can combine subexpressions by delimiting them by commas:\n" .
         "    1,2,3\n" .
         "      Do steps 1, 2, and 3\n" .
         "    ^5-,10\n" .
         "      Don't do step 5 or any step after that, except for 10\n" .
         "    3-20,^5-7,^14\n" .
         "      Do steps 3 through 20, except for steps 5, 6, 7, and 14\n";
}

sub is_option {
  my $arg = shift;
  return $arg =~ m/^--/ || $arg eq "-h";
}

sub remove_end_slash {
  my $val = shift;
  if ($val =~ /^(.*)\/$/) {
    $val = $1
  }
  return $val;
}

sub normalizePath {
  my $val = shift;
  if ($val =~ /../) {
    $val = Cwd::abs_path($val);
  }
  return remove_end_slash($val);
}

sub news_contents_excerpt($) {
  my $version = quotemeta(shift);
  my @lines;
  open(my $news, "NEWS.md") or die "Can't read NEWS.md file";
  my $saw_version = 0;
  while (<$news>) {
    if (/^## Version $version of OpenDDS/) {
      $saw_version = 1;
      next;
    } elsif (/^##[^#]/ && $saw_version) { # Until we come to the next h2
      last;
    }
    if ($saw_version) {
      if ($saw_version < 2) { # Skip Timestamp Line
        $saw_version++;
        next;
      }
      push (@lines, $_);
    }
  }
  close $news;
  return join("",@lines);
}

sub email_announce_contents {
  my $settings = shift();

  my $result =
    "OpenDDS version $settings->{version} is now available for download at $settings->{download_url}/\n" .
    "\n";

  if (!$settings->{skip_devguide}) {
    $result .=
      "An updated version of the OpenDDS Developer's Guide PDF is available\n" .
      "from $settings->{download_url}/$settings->{devguide_ver}\n" .
      "\n";
  }

  $result .=
    "Updates in this version:\n" .
    "(This is an excerpt of the NEWS, for full change information " .
      "see ChangeLog within the source distribution)\n" .
    news_contents_excerpt($settings->{version});

  return $result;
}

# Run command, returns 0 if there was an error
sub run_command ($;$) {
  my $command = shift;
  my $ignore_failure = shift;
  if (!defined $ignore_failure) {
      $ignore_failure = 0;
  }

  print "$command\n";
  if (system ($command)) {
      if (!$ignore_failure) {
        my $error_message;
        if ($? == -1) {
            $error_message = "Failed to Run: $!";
        } elsif ($? & 127) {
            $error_message = sprintf ("Exited on Signal %d, %s coredump",
              ($? & 127),  ($? & 128) ? 'with' : 'without');
        } else {
            $error_message = sprintf ("Returned %d", $? >> 8);
        }
        print STDERR "Command \"$command\" $error_message\n";
      }
      return $ignore_failure;
  }
  return 1;
}

sub yes_no {
  while (<STDIN>) {
    chomp;
    if ($_ eq "n") {
      return 0;
    } elsif ($_ eq "y") {
      return 1;
    } else {
      print "Please answer y or n. ";
    }
  }
}

############################################################################

my $step_subexpr_re = qr/(\^?)(\d*)(-?)(\d*)/;
my $step_expr_re = qr/^${step_subexpr_re}(,${step_subexpr_re})*$/;

sub parse_step_expr {
  my ($no_steps, $expr) = @_;
  if (not ($expr =~ $step_expr_re)) {
    die "Invalid Steps: $expr, see --help for the notation";
  }

  my %tmp_steps = ();
  foreach my $i (1..$no_steps) {
    $tmp_steps{$i} = 0;
  }
  my $first = 1;

  foreach my $subexpr (split(/,/, $expr)) {
    if (not $subexpr) {
      die "Invalid Steps: $expr, see --help for the notation";
    }
    $subexpr =~ $step_subexpr_re;

    my $negative = $1;
    # If first element is negative, then the expression is negative overall,
    # start with all steps.
    # If not, then the expression is positive overall, start with nothing.
    if ($negative && $first) {
      foreach my $i (1..$no_steps) {
        $tmp_steps{$i} = 1;
      }
    }
    $first = 0;

    my $x;
    if ($2) {
      $x = int($2);
      if ($x > $no_steps) {
        die "$x is greater than the total number of steps, $no_steps";
      }
    } else {
      $x = 1;
    }

    my $range = $3;

    my $y;
    if ($4) {
      $y = int($4);
      if ($y > $no_steps) {
        die "$y is greater than the total number of steps, $no_steps";
      }
      if ($x > $y) {
        die "Invalid range in steps: $x-$y";
      }
    } elsif ($range) {
      $y = $no_steps;
    } else {
      $y = $x;
    }

    # Apply The Subexpression
    foreach my $i ($x..$y) {
      $tmp_steps{$i} = $negative ? 0 : 1;
    }
  }

  my @steps = ();
  foreach my $i (0..$no_steps) {
    push(@steps, $i) if ($tmp_steps{$i});
  }

  return @steps;
}

sub parse_version {
  my $version = shift;
  my %result = ();
  if ($version =~ /([0-9]+)\.([0-9]+)\.?([0-9]+)?/) {
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

sub remedy_git_status_clean {
  my $settings = shift();
  my $version = $settings->{version};
  system("git diff") == 0 or die "Could not execute: git diff";
  print "Would you like to add and commit these changes [y/n]? ";
  return 0 if (!yes_no());
  system("git add docs/history/ChangeLog-$version") == 0 or die "Could not execute: git add docs/history/ChangeLog-$version";
  system("git add -u") == 0 or die "Could not execute: git add -u";
  my $message = "OpenDDS Release $version";
  system("git commit -m '" . $message . "'") == 0 or die "Could not execute: git commit -m";
  return 1;
}
############################################################################
sub verify_update_version_file {
  my $settings = shift();
  my $version = $settings->{version};
  my $correct = 0;
  my $status = open(VERSION, 'VERSION.txt');
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
  return "VERSION.txt file needs updating with current version"
}

sub remedy_update_version_file {
  my $settings = shift();
  my $version = $settings->{version};
  print "  >> Updating VERSION.txt file for $version\n";
  my $timestamp = $settings->{timestamp};
  my $outline = "This is OpenDDS version $version, released $timestamp";
  my $corrected = 0;
  open(VERSION, "+< VERSION.txt") or die "Opening: $!";
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
    next unless /^$git_name_prefix([\d\.]*)/;
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

  open(GITLOG, "git log $prev_tag..HEAD --name-status --date=raw |") or die "Opening $!";
  while (<GITLOG>) {
    chomp;
    s/\t/  /g;
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

# See $DDS_ROOT/.mailmap file for to add individual corrections

my $github_email_re = qr/^(?:\d+\+)?(.*)\@users.noreply.github.com$/;

my @global_authors = ();

sub search_authors {
  my @authors = @{shift()};
  my $email = shift || 0;
  my $name = shift || 0;

  foreach my $author (@authors) {
    if ($email) {
      if ($author->{email} =~ $email) {
        return $author;
      }
    }
    if ($name) {
      if ($author->{name} =~ $name) {
        return $author;
      }
    }
  }

  return undef;
}

sub name_key ($) {
  my @name = split(/\s+/, shift);
  return lc($name[-1]);
}

sub get_authors {
  my @authors = ();

  open(CHANGELOG, "git shortlog -se |") or die "Opening $!";
  while (<CHANGELOG>) {
    if (/^\s*\d+\s+([^<>]+)<(.*)>/) {
      my $name = $1;
      my $email = $2;
      $name =~ s/^\s+|\s+$//g;
      $email =~ s/^\s+|\s+$//g;

      # Ignore the entry if it contains [bot]
      next if ($email =~ /\[bot\]/);

      # Ignore the entry if it definitely doesn't have a TLD
      next if (not $email =~ /@.*\..+/);

      # Prefer New OCI Domain to Old
      if ($email =~ /^(.*)\@objectcomputing.com$/) {
        my $ociusername = quotemeta($1);
        my $author = search_authors(\@authors, qr/^$ociusername\@ociweb.com$/);
        if (defined $author) {
          $author->{email} = $email;
          next;
        }
      }
      if ($email =~ /^(.*)\@ociweb.com$/) {
        my $ociusername = quotemeta($1);
        my $author = search_authors(\@authors, qr/^$ociusername\@objectcomputing.com$/);
        if (defined $author) {
          next;
        }
      }

      # Prefer Actual Emails to Github Accounts
      my $name_quote = quotemeta($name);
      my $author = search_authors(\@authors, 0, qr/${name_quote}/);
      if (defined $author) {
        my $this_is_gh = $email =~ /$github_email_re/;
        my $other_is_gh = $author->{email} =~ /$github_email_re/;
        if ((!$this_is_gh) && $other_is_gh) {
          $author->{email} = $email
        }
        next;
      }

      push (@authors, {email => $email, name => $name});
    }
  }
  close (CHANGELOG);

  # Replace Github Account emails with url to their profiles
  foreach my $author (@authors) {
    if ($author->{email} =~ /$github_email_re/) {
      $author->{email} = "https://github.com/$1";
    }
  }

  return sort { name_key($a->{name}) cmp name_key($b->{name}) } @authors;
}

sub verify_authors {
  my $settings = shift();
  @global_authors = get_authors() if (!@global_authors);

  my $tmp_authors_path = "$settings->{workspace}/AUTHORS";
  open(my $file, '>', $tmp_authors_path) or die ("Could not open $tmp_authors_path: $!");
  foreach my $author (@global_authors) {
    print $file "$author->{name} <$author->{email}>\n";
  }
  close($file);

  my $command = "git --no-pager diff --no-index AUTHORS $tmp_authors_path";
  my $diff = `$command`;
  if ($?) {
    print("$command\n");
    print "Authors needs ammending:\n$diff";
    return 0;
  } else {
    return 1;
  }
}

sub message_authors {
  my $settings = shift();
  return "AUTHORS file needs updating\n";
}

sub remedy_authors {
  open(my $authors, '>', "AUTHORS") or die "Opening $!";
  foreach my $author (@global_authors) {
    print $authors "$author->{name} <$author->{email}>\n";
  }
  close $authors;
  return 1;
}

############################################################################
sub verify_news_file_section {
  my $settings = shift();
  my $version = $settings->{version};
  my $status = open(NEWS, 'NEWS.md');
  my $metaversion = quotemeta($version);
  my $has_version = 0;
  while (<NEWS>) {
    if ($_ =~ /^## Version $metaversion of OpenDDS/) {
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
  insert_news_template($version, $settings->{timestamp});
  return 0;
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
    if ($_ =~ /^## Version $metaversion of OpenDDS/) {
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

sub verify_news_timestamp {
  my $marker = quotemeta($timestamp_marker);
  my $has_marker = 0;
  open(my $news_file, 'NEWS.md');
  while (<$news_file>) {
    if ($_ =~ /$marker/) {
      $has_marker = 1;
    }
  }
  close($news_file);
  return !$has_marker;
}

sub message_news_timestamp {
  return "The NEWS.md section for this release needs its timestamp inserted";
}

sub remedy_news_timestamp {
  my $settings = shift();
  my $timestamp = $settings->{timestamp};
  my $marker = quotemeta($timestamp_marker);

  # Read News File
  open(my $news_file, '<', "NEWS.md");
  my @lines = <$news_file>;
  close $news_file;

  # Insert Template
  foreach my $line (@lines) {
    last if $line =~ s/$marker/$timestamp/;
  }

  # Write News File
  open($news_file, '>', "NEWS.md");
  foreach my $line (@lines) {
    print $news_file ($line);
  }
  close($news_file);
  return 1;
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
  open(GIT, "git ls-remote --tags $settings->{remote} |") or die "Opening $!";
  while (<GIT>) {
    chomp;
    if (/$target/) {
      $found = 1;
    }
  }
  close GIT;

  my $unpused_commits = 0;
  open(GIT, "git log $settings->{remote}/$settings->{branch}..HEAD |") or die "Opening $!";
  while (<GIT>) {
    chomp;
    if (/^commit/) {
      $unpused_commits++;
    }
  }
  close GIT;

  return $found && !$unpused_commits;
}

sub message_git_changes_pushed {
  my $settings = shift();
  return "Changes and tags need to be pushed to $settings->{remote}\n";
}

sub remedy_git_changes_pushed {
  my $settings = shift();
  my $push_latest_release = shift();
  my $remote = $settings->{remote};
  print "pushing code\n";
  my $result = system("git push $remote $settings->{branch}");
  print "pushing tags with git push $remote --tags\n";
  $result = $result || system("git push $remote --tags");
  if ($push_latest_release) {
    print "pushing latest-release branch\n";
    $result = $result || !run_command("git push --force $remote $settings->{branch}:latest-release");
  }
  return !$result;
}

############################################################################
sub verify_git_tag {
  my $settings = shift();
  my $found = 0;
  open(GITTAG, "git tag --list '$git_name_prefix*' |") or die "Opening $!";
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

sub verify_create_release_branch {
  my $settings = shift();
  my $branch = quotemeta($settings->{release_branch});
  my $found;
  open(GIT, "git branch |") or die "Opening $!";
  while (<GIT>) {
    chomp;
    if (/$branch/) {
      $found = 1;
    }
  }
  close GIT;
  return $found;
}

sub message_create_release_branch {
  my $settings = shift();
  print "Create a Git Branch Called $settings->{release_branch}\n";
}

sub remedy_create_release_branch {
  my $settings = shift();
  return run_command("git branch --force $settings->{release_branch} $settings->{git_tag}");
}

############################################################################

sub verify_push_release_branch {
  my $settings = shift();
  my $branch = quotemeta($settings->{release_branch});
  my $remote = quotemeta($settings->{remote});
  my $found;
  open(GIT, "git branch -r |") or die "Opening $!";
  while (<GIT>) {
    chomp;
    if (/$remote\/$branch/) {
      $found = 1;
    }
  }
  close GIT;
  return $found;
}

sub message_push_release_branch {
  my $settings = shift();
  print "Push a Git Branch Called $settings->{release_branch} to $settings->{remote}\n";
}

sub remedy_push_release_branch {
  my $settings = shift();
  return run_command("git push --set-upstream $settings->{remote} --force $settings->{release_branch}");
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

  if (!run_command("git worktree add $settings->{clone_dir} $settings->{git_tag}")) {
    print "Couldn't create temporary website worktree!\n";
    return 0;
  }

  return 1;
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
  my $file = "$settings->{workspace}/$settings->{tgz_src}";
  my $good = 0;
  if (-f $file) {
    # Check if it is in the right format
    my $basename = basename($settings->{clone_dir});
    open(TGZ, "gzip -c -d $file | tar -tvf - |") or die "Opening $!";
    my $target = join("/", $basename, 'VERSION.txt');
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
  my $file = "$settings->{workspace}/$settings->{tgz_src}";
  if (!-f $file) {
    return "Could not find file $file";
  } else {
    return "File $file is not in the right format";
  }
}

sub remedy_tgz_source {
  my $settings = shift();
  my $tempdir = tempdir(CLEANUP => 1);
  my $file = "$settings->{workspace}/$settings->{tgz_src}";
  my $tfile = join("/", $tempdir, $settings->{tgz_src});
  my $curdir = getcwd;
  my $result = 0;
  my $basename = basename($settings->{clone_dir});

  print "Copying $settings->{clone_dir} to $tempdir ($result)\n";
  $result = $result || system("cp -aR $settings->{clone_dir} $tempdir");

  chdir($tempdir);
  print "Removing git-specific directories ($result)\n";
  $result = $result || system("find . -name '.git*' | xargs rm -rf");
  print "Creating tar file ($result)\n";
  $result = $result || system("tar -cf $settings->{tar_src} $basename");
  print "Gzipping file $settings->{tar_src} ($result)\n";
  $result = $result || system("gzip -9 $settings->{tar_src}");
  chdir($curdir);

  print "Move gzip file ($result)\n";
  $result = $result || system("mv $tfile $file");

  return !$result;
}
############################################################################
sub verify_zip_source {
  my $settings = shift();
  my $file = join("/", $settings->{workspace}, $settings->{zip_src});
  return (-f $file);
}

sub message_zip_source {
  my $settings = shift();
  my $file = join("/", $settings->{workspace}, $settings->{zip_src});
  return "Could not find file $file";
}

sub remedy_zip_source {
  my $settings = shift();
  my $tempdir = tempdir(CLEANUP => 1);
  my $file = join("/", $settings->{workspace}, $settings->{zip_src});
  my $tfile = join("/", $tempdir, $settings->{zip_src});
  my $curdir = getcwd;
  my $result = 0;

  print "Copying $settings->{clone_dir} to $tempdir ($result)\n";
  $result = $result || system("cp -aR $settings->{clone_dir} $tempdir");

  chdir($tempdir);
  print "Removing git-specific directories ($result)\n";
  $result = $result || system("find . -name '.git*' | xargs rm -rf");
  print "Converting source files to Windows line endings ($result)\n";
  my $converter = new ConvertFiles();
  my ($stat, $error) = $converter->convert(".");
  if (!$stat) {
    print $error;
    $result = 1;
  }
  print "Creating file $settings->{zip_src} ($result)\n";
  $result = $result || system("zip $settings->{zip_src} -9 -qq -r . -x '.git*'");
  chdir($curdir);

  print "Move zip file ($result)\n";
  $result = $result || system("mv $tfile $file");

  return !$result;
}
############################################################################
sub verify_md5_checksum{
  my $settings = shift();
  my $file = join("/", $settings->{workspace}, $settings->{md5_src});
  return (-f $file);
}

sub message_md5_checksum{
  return "Generate the MD5 checksum file";
}

sub remedy_md5_checksum{
  my $settings = shift();
  print "Creating file $settings->{md5_src}\n";
  my $md5_file = join("/", $settings->{workspace}, $settings->{md5_src});
  my $tgz_file = join("/", $settings->{workspace}, $settings->{tgz_src});
  my $zip_file = join("/", $settings->{workspace}, $settings->{zip_src});
  return !system("md5sum $tgz_file $zip_file > $md5_file");
}
############################################################################

sub verify_download_ace_tao {
  my $settings = shift();
  my $archive = "$settings->{workspace}/$ace_tao_filename";
  return -f $archive;
}

sub message_download_ace_tao {
  return "Download OCI ACE/TAO";
}

sub remedy_download_ace_tao {
  my $settings = shift();
  my $archive = "$settings->{workspace}/$ace_tao_filename";
  print "Downloading $ace_tao_url...\n";
  my $code = getstore($ace_tao_url, $archive);
  if ($code != 200) {
    die "Download failed, response code was $code.\n";
  }
  print "Download Finished!\n";
  return 1;
}

############################################################################

sub verify_extract_ace_tao {
  my $settings = shift();
  return -f "$settings->{ace_root}/ace/ACE.h";
}

sub message_extract_ace_tao {
  return "Extract OCI ACE/TAO";
}

sub remedy_extract_ace_tao {
  my $settings = shift();
  my $archive = "$settings->{workspace}/$ace_tao_filename";
  print "Extracting $archive...\n";
  my $result = run_command("tar xzf $archive -C $settings->{workspace}");
  return !$result;
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
  my $curdir = getcwd;
  $ENV{DDS_ROOT} = $settings->{clone_dir};
  $ENV{ACE_ROOT} = $settings->{ace_root};
  chdir($ENV{DDS_ROOT});
  my $result = run_command("$ENV{DDS_ROOT}/tools/scripts/generate_combined_doxygen.pl . -is_release");
  chdir($curdir);
  return !$result;
}
############################################################################
sub verify_tgz_doxygen {
  my $settings = shift();
  my $file = join("/", $settings->{workspace}, $settings->{tgz_dox});
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
  my $file = join("/", $settings->{workspace}, $settings->{tgz_dox});
  return "Could not find file $file";
}

sub remedy_tgz_doxygen {
  my $settings = shift();
  my $file = join("/", $settings->{workspace}, $settings->{tar_dox});
  my $curdir = getcwd;
  chdir($settings->{clone_dir});
  print "Creating file $settings->{tar_dox}\n";
  my $result = system("tar -cf ../$settings->{tar_dox} html");
  if (!$result) {
    print "Gzipping file $settings->{tar_dox}\n";
    $result = system("gzip -9 ../$settings->{tar_dox}");
  }
  chdir($curdir);
  return !$result;
}
############################################################################
sub verify_zip_doxygen {
  my $settings = shift();
  my $file = join("/", $settings->{workspace}, $settings->{zip_dox});
  return (-f $file);
}

sub message_zip_doxygen {
  my $settings = shift();
  my $file = join("/", $settings->{workspace}, $settings->{zip_dox});
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
  my $result = system("zip $file -9 -qq -r dds");
  chdir($curdir);
  return !$result;
}
############################################################################
sub verify_devguide {
  my $settings = shift();
  return (-f "$settings->{workspace}/$settings->{devguide_ver}" &&
          -f "$settings->{workspace}/$settings->{devguide_lat}");
}

sub message_devguide {
  my $settings = shift();
  return "Devguide missing";
}

sub remedy_devguide {
  my $settings = shift();
  my $devguide_url = "svn+ssh://svn.ociweb.com/devguide/opendds/trunk/$settings->{devguide_ver}";
  print "Downloading devguide\n$devguide_url\n";
  my $result = system("svn cat $devguide_url > $settings->{workspace}/$settings->{devguide_ver}");
  $result = $result || system("svn cat $devguide_url > $settings->{workspace}/$settings->{devguide_lat}");
  return !$result;
}

############################################################################
sub get_release_files {
  my $settings = shift();
  my @files = (
      $settings->{tgz_src},
      $settings->{zip_src},
      $settings->{md5_src},
  );
  if (!$settings->{skip_devguide}) {
    push(@files , $settings->{devguide_ver});
    push(@files , $settings->{devguide_lat});
  }
  if (!$settings->{skip_doxygen}) {
    push(@files , $settings->{tgz_dox});
    push(@files , $settings->{zip_dox});
  }
  return @files;
}

sub verify_ftp_upload {
  my $settings = shift();
  my $content = get("$settings->{download_url}/");

  if (not defined $content) {
    print "Error getting current release files from $settings->{download_url}/\n";
    return 0;
  }

  foreach my $file (get_release_files($settings)) {
    if ($content =~ /$file/) {
    } else {
      print "$file not found at $settings->{download_url}/\n";
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

  my @new_release_files = get_release_files($settings);
  my %release_file_map = map { $_ => 1 } @new_release_files;

  # Identify Old Versioned Release Files Using the New Ones
  my @release_file_re = ();
  my $quoted_base_name_prefix = quotemeta($base_name_prefix);
  my $version_re = qr/\d+\.\d+(.\d+)?/;
  foreach my $file (@new_release_files) {
    my $re;
    my $versioned;
    if ($file =~ m/^${quoted_base_name_prefix}${version_re}(.*)$/) {
      my $suffix = ${2} || "";
      $re = qr/^${quoted_base_name_prefix}${version_re}${suffix}$/;
      $versioned = 1;
    } else {
      $re = qr/^$file$/;
      $versioned = 0;
    }
    push(@release_file_re, {re => $re, versioned => $versioned});
  }

  # And Move or Delete Them
  foreach my $file (@current_release_files) {
    foreach my $file_info (@release_file_re) {
      if ($file =~ $file_info->{re}) {
        if ($file_info->{versioned}) {
          my $new_path = $PRIOR_RELEASE_PATH . $file;
          print "moving $file to $new_path\n";
          $ftp->rename($file, $new_path) or die "Could not rename $file to $new_path: " . $ftp->message;
        } else {
          print "deleting $file\n";
          $ftp->delete($file) or die "Could not delete $file: " . $ftp->message;
        }
        last;
      }
    }
  }

  # upload new release files
  foreach my $file (@new_release_files) {
    print "uploading $file\n";
    my $local_file = join("/", $settings->{workspace}, $file);
    $ftp->put($local_file, $file) or die "Failed to upload $file: " . $ftp->message();
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

  my $ph = Pithub::Repos::Releases->new(
      user => $settings->{github_user},
      repo => $settings->{github_repo},
      token => $settings->{github_token}
  );
  my $text =
    "**Download $settings->{zip_src} (Windows) or $settings->{tgz_src} (Linux/macOS) " .
      "instead of \"Source code (zip)\" or \"Source code (tar.gz)\".**\n\n" .
    news_contents_excerpt($settings->{version});
  my $release = $ph->create(
      data => {
          name => 'OpenDDS ' . $settings->{version},
          tag_name => $settings->{git_tag},
          body => $text
      }
  );
  unless ( $release->success ) {
    printf "error accessing github: %s\n", $release->response->status_line;
    $rc = 0;
  } else {
    for my $f ($settings->{tgz_src},
               $settings->{zip_src}) {
      my $p = "$settings->{workspace}/$f";
      open(my $fh, $p) or die "Can't open";
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
  my $settings = shift();
  my $remote = $settings->{remote};
  my $status;

  # fetch remote branches so we have up to date versions
  if ($status && !run_command("git fetch $remote website-next-release")) {
    print STDERR "Couldn't fetch website-next-release!\n";
    return 0;
  }
  if ($status && !run_command("git fetch $remote gh-pages")) {
    print STDERR "Couldn't fetch gh-pages!\n";
    return 0;
  }

  $status = open(GITDIFF, 'git diff ' . $remote  . '/website-next-release ' . $remote . '/gh-pages|');
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
  my $settings = shift();
  my $remote = $settings->{remote};
  return "$remote/website-next-release branch needs to merge into $remote/gh-pages branch";
}

sub remedy_website_release {
  my $settings = shift();
  print "Releasing gh-pages website\n";
  my $worktree = "$settings->{workspace}/temp_website";
  my $remote = $settings->{remote};
  my $rm_worktree = 0;
  my $status = 1;

  if (!run_command("git worktree add $worktree gh-pages")) {
    print STDERR
      "Couldn't create temporary website worktree!\n";
    $status = 0;
  }
  $rm_worktree = 1;
  my $prev_dir = getcwd;
  chdir $worktree;

  # Get the two branches merged with each other
  if ($status && !run_command("git pull")) {
    print STDERR "Couldn't pull gh-pages!\n";
    $status = 0;
  }
  if ($status && !run_command("git checkout website-next-release")) {
    print STDERR "Couldn't checkout website-next-release!\n";
    $status = 0;
  }
  if ($status && !run_command("git pull")) {
    print STDERR "Couldn't pull website-next-release!\n";
    $status = 0;
  }
  if ($status && !run_command("git merge gh-pages")) {
    print STDERR "Couldn't merge website-next-release into gh-pages\n";
    $status = 0;
  }
  if ($status && !run_command("git push")) {
    print STDERR "Couldn't push website-next-release!\n";
    $status = 0;
  }

  if ($status && !run_command("git checkout gh-pages")) {
    print STDERR "Couldn't checkout gh-pages!\n";
    $status = 0;
  }
  if ($status && !run_command("git merge website-next-release")) {
    print STDERR "Couldn't merge website-next-release into gh-pages\n";
    $status = 0;
  }
  if ($status && !run_command("git push")) {
    print STDERR "Couldn't push gh-pages!\n";
    $status = 0;
  }

  chdir $prev_dir;
  if ($rm_worktree && !run_command("git worktree remove $worktree")) {
    print STDERR "Couldn't remove temporary worktree for website merge!\n";
  }

  if (!$status) {
    print STDERR
      "You must merge website-next-release with gh-pages on your own before you " .
      "can continue\n";
  }

  return $status;
}

############################################################################
sub verify_email_list {
  # Can't verify
  my $settings = shift;
  print 'Email this text to dds-release-announce@ociweb.com' . "\n\n" .
    email_announce_contents($settings);
  return 1;
}

sub message_email_dds_release_announce {
  return 'Email needs to be sent to dds-release-announce@ociweb.com';
}

sub remedy_email_dds_release_announce {
  return 1;
}

############################################################################
sub verify_news_template_file_section {
  my $settings = shift();
  my $status = open(NEWS, 'NEWS.md');
  my $has_news_template = 0;
  while (<NEWS>) {
    if ($_ =~ /^## Version X.Y.Z of OpenDDS/) {
      $has_news_template = 1;
    }
  }
  close(NEWS);

  return ($has_news_template);
}

sub message_news_template_file_section {
  return "Template for next release in NEWS.md is missing";
}

sub remedy_news_template_file_section {
  my $settings = shift();

  insert_news_template("X.Y.Z", $timestamp_marker);

  return 1;
}

############################################################################
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

my $status = 0;

my $print_help = any_arg_is("-h") || any_arg_is("--help");
my $print_list = any_arg_is("--list");
my $is_micro = any_arg_is("--micro");
my $github_user = string_arg_value("--github-user") || $default_github_user;
my $branch = string_arg_value("branch");

if ($is_micro && !$branch) {
  die "For micro releases, you must define the branch you want to use with --branch";
}

# Process Workspace Argument
my $workspace = $ARGV[0] || "";
$workspace = "" if is_option($workspace);
if ($workspace) {
  $workspace = normalizePath($workspace);
  if (!($print_help || $print_list) && !(-d $workspace)) {
    make_path($workspace) or die("Failed to create workspace directory: $workspace");
  }
}

# Validate and Normalize Version String
my %parsed_version = parse_version($ARGV[1] || "");
my $version = "";
if (%parsed_version) {
  if ($parsed_version{micro} > 0) {
    $version = sprintf("%d.%d.%d",
      $parsed_version{major}, $parsed_version{minor}, $parsed_version{micro});
  } else {
    $version = sprintf("%d.%d",
      $parsed_version{major}, $parsed_version{minor});
  }
}

my $base_name = "${base_name_prefix}${version}";

my $release_branch = "";
if (!$is_micro && %parsed_version) {
  $release_branch = sprintf("%s%s%d.%d",
    $release_branch_prefix, $git_name_prefix,
    $parsed_version{major}, $parsed_version{minor});
}

my %global_settings = (
    list         => $print_list,
    remedy       => any_arg_is("--remedy"),
    force        => any_arg_is("--force"),
    micro        => $is_micro,
    remote       => string_arg_value("--remote") || $default_remote,
    branch       => $branch || $default_branch,
    release_branch => $release_branch,
    github_user  => $github_user,
    version      => $version,
    base_name    => $base_name,
    git_tag      => "${git_name_prefix}${version}",
    clone_dir    => join("/", $workspace, ${base_name}),
    tar_src      => "${base_name}.tar",
    tgz_src      => "${base_name}.tar.gz",
    zip_src      => "${base_name}.zip",
    md5_src      => "${base_name}.md5",
    tar_dox      => "${base_name}-doxygen.tar",
    tgz_dox      => "${base_name}-doxygen.tar.gz",
    zip_dox      => "${base_name}-doxygen.zip",
    devguide_ver => "${base_name}.pdf",
    devguide_lat => "${base_name_prefix}latest.pdf",
    timestamp    => POSIX::strftime($timefmt, gmtime),
    git_url      => "git\@github.com:${github_user}/${repo_name}.git",
    github_repo  => $repo_name,
    github_token => $ENV{GITHUB_TOKEN},
    ftp_user     => $ENV{FTP_USERNAME},
    ftp_password => $ENV{FTP_PASSWD},
    ftp_host     => $ENV{FTP_HOST},
    changelog    => "docs/history/ChangeLog-$version",
    modified     => {
        "NEWS.md" => 1,
        "PROBLEM-REPORT-FORM" => 1,
        "VERSION.txt" => 1,
        "dds/Version.h" => 1,
        "docs/history/ChangeLog-$version" => 1,
        "tools/scripts/gitrelease.pl" => 1,
    },
    skip_ftp     => any_arg_is("--skip-ftp"),
    skip_devguide=> any_arg_is("--skip-devguide") || $is_micro,
    skip_doxygen => any_arg_is("--skip-doxygen") || $is_micro,
    skip_website => any_arg_is("--skip-website") || $is_micro,
    workspace    => $workspace,
    download_url => remove_end_slash(string_arg_value("--download-url")) || $default_download_url,
    ace_root     => "$workspace/$ace_root"
);

if (%parsed_version) {
  $global_settings{major_version} = $parsed_version{major};
  $global_settings{minor_version} = $parsed_version{minor};
  $global_settings{micro_version} = $parsed_version{micro};
}

my @release_steps  = (
  {
    name    => 'Update VERSION.txt',
    verify  => sub{verify_update_version_file(@_)},
    message => sub{message_update_version_file(@_)},
    remedy  => sub{remedy_update_version_file(@_)},
    can_force => 1,
  },
  {
    name    => 'Update Version.h',
    verify  => sub{verify_update_version_h_file(@_)},
    message => sub{message_update_version_h_file(@_)},
    remedy  => sub{remedy_update_version_h_file(@_)},
    can_force => 1,
  },
  {
    name    => 'Update PROBLEM-REPORT-FORM',
    verify  => sub{verify_update_prf_file(@_)},
    message => sub{message_update_prf_file(@_)},
    remedy  => sub{remedy_update_prf_file(@_)},
    can_force => 1,
  },
  {
    name    => 'Verify remote arg',
    verify  => sub{verify_git_remote(@_)},
    message => sub{message_git_remote(@_)},
    remedy  => sub{override_git_remote(@_)},
  },
  {
    name    => 'Create ChangeLog',
    verify  => sub{verify_changelog(@_)},
    message => sub{message_changelog(@_)},
    remedy  => sub{remedy_changelog(@_)},
    can_force => 1,
  },
  {
    name    => 'Update Authors',
    verify  => sub{verify_authors(@_)},
    message => sub{message_authors(@_)},
    remedy  => sub{remedy_authors(@_)},
    can_force => 1,
  },
  {
    name    => 'Add NEWS Section',
    verify  => sub{verify_news_file_section(@_)},
    message => sub{message_news_file_section(@_)},
    remedy  => sub{remedy_news_file_section(@_)},
    can_force => 1,
  },
  {
    name    => 'Update NEWS Section',
    verify  => sub{verify_update_news_file(@_)},
    message => sub{message_update_news_file(@_)},
    can_force => 1,
  },
  {
    name    => 'Update NEWS Section Timestamp',
    verify  => sub{verify_news_timestamp(@_)},
    message => sub{message_news_timestamp(@_)},
    remedy  => sub{remedy_news_timestamp(@_)},
    can_force => 1,
  },
  {
    name    => 'Commit changes to GIT',
    verify  => sub{verify_git_status_clean(@_, 1)},
    message => sub{message_commit_git_changes(@_)},
    remedy  => sub{remedy_git_status_clean(@_)}
  },
  {
    name    => 'Create git tag',
    verify  => sub{verify_git_tag(@_)},
    message => sub{message_git_tag(@_)},
    remedy  => sub{remedy_git_tag(@_)}
  },
  {
    name    => 'Verify changes pushed',
    prereqs => ['Verify remote arg'],
    verify  => sub{verify_git_changes_pushed(@_, 1)},
    message => sub{message_git_changes_pushed(@_)},
    remedy  => sub{remedy_git_changes_pushed(@_, 1)}
  },
  {
    name    => 'Create Release Branch',
    verify  => sub{verify_create_release_branch(@_)},
    message => sub{message_create_release_branch(@_)},
    remedy  => sub{remedy_create_release_branch(@_)},
    skip    => !$global_settings{release_branch},
  },
  {
    name    => 'Push Release Branch',
    prereqs => ['Create Release Branch', 'Verify remote arg'],
    verify  => sub{verify_push_release_branch(@_)},
    message => sub{message_push_release_branch(@_)},
    remedy  => sub{remedy_push_release_branch(@_)},
    skip    => !$global_settings{release_branch},
  },
  {
    name    => 'Clone tag',
    prereqs => ['Verify remote arg'],
    verify  => sub{verify_clone_tag(@_)},
    message => sub{message_clone_tag(@_)},
    remedy  => sub{remedy_clone_tag(@_)}
  },
  {
    name    => 'Move changelog',
    verify  => sub{verify_move_changelog(@_)},
    message => sub{message_move_changelog(@_)},
    remedy  => sub{remedy_move_changelog(@_)}
  },
  {
    name    => 'Create unix release archive',
    verify  => sub{verify_tgz_source(@_)},
    message => sub{message_tgz_source(@_)},
    remedy  => sub{remedy_tgz_source(@_)}
  },
  {
    name    => 'Create windows release archive',
    verify  => sub{verify_zip_source(@_)},
    message => sub{message_zip_source(@_)},
    remedy  => sub{remedy_zip_source(@_)}
  },
  {
    name    => 'Download OCI ACE/TAO',
    verify  => sub{verify_download_ace_tao(@_)},
    message => sub{message_download_ace_tao(@_)},
    remedy  => sub{remedy_download_ace_tao(@_)},
    skip    => $global_settings{skip_doxygen},
  },
  {
    name    => 'Extract OCI ACE/TAO',
    prereqs => ['Download OCI ACE/TAO'],
    verify  => sub{verify_extract_ace_tao(@_)},
    message => sub{message_extract_ace_tao(@_)},
    remedy  => sub{remedy_extract_ace_tao(@_)},
    skip    => $global_settings{skip_doxygen},
  },
  {
    name    => 'Generate doxygen',
    prereqs => ['Extract OCI ACE/TAO'],
    verify  => sub{verify_gen_doxygen(@_)},
    message => sub{message_gen_doxygen(@_)},
    remedy  => sub{remedy_gen_doxygen(@_)},
    skip    => $global_settings{skip_doxygen},
  },
  {
    name    => 'Create unix doxygen archive',
    prereqs => ['Generate doxygen'],
    verify  => sub{verify_tgz_doxygen(@_)},
    message => sub{message_tgz_doxygen(@_)},
    remedy  => sub{remedy_tgz_doxygen(@_)},
    skip    => $global_settings{skip_doxygen},
  },
  {
    name    => 'Create windows doxygen archive',
    prereqs => ['Generate doxygen'],
    verify  => sub{verify_zip_doxygen(@_)},
    message => sub{message_zip_doxygen(@_)},
    remedy  => sub{remedy_zip_doxygen(@_)},
    skip    => $global_settings{skip_doxygen},
  },
  {
    name    => 'Create md5 checksum',
    verify  => sub{verify_md5_checksum(@_)},
    message => sub{message_md5_checksum(@_)},
    remedy  => sub{remedy_md5_checksum(@_)},
  },
  {
    name    => 'Download Devguide',
    verify  => sub{verify_devguide(@_)},
    message => sub{message_devguide(@_)},
    remedy  => sub{remedy_devguide(@_)},
    skip    => $global_settings{skip_devguide},
  },
  {
    name    => 'Upload to FTP Site',
    verify  => sub{verify_ftp_upload(@_)},
    message => sub{message_ftp_upload(@_)},
    remedy  => sub{remedy_ftp_upload(@_)},
    skip    => $global_settings{skip_ftp},
  },
  {
    name    => 'Upload to GitHub',
    verify  => sub{verify_github_upload(@_)},
    message => sub{message_github_upload(@_)},
    remedy  => sub{remedy_github_upload(@_)}
  },
  {
    name    => 'Release Website',
    verify  => sub{verify_website_release(@_)},
    message => sub{message_website_release(@_)},
    remedy  => sub{remedy_website_release(@_)},
    skip    => $global_settings{skip_website},
  },
  {
    name    => 'Add NEWS Template Section',
    verify  => sub{verify_news_template_file_section(@_)},
    message => sub{message_news_template_file_section(@_)},
    remedy  => sub{remedy_news_template_file_section(@_)},
    skip    => $global_settings{micro},
  },
  {
    name    => 'Commit NEWS Template Section',
    verify  => sub{verify_git_status_clean(@_, 1)},
    message => sub{message_commit_git_changes(@_)},
    remedy  => sub{remedy_git_status_clean(@_)},
    skip    => $global_settings{micro},
  },
  {
    name    => 'Push NEWS Template Section',
    prereqs => ['Verify remote arg'],
    verify  => sub{verify_git_changes_pushed(@_, 1)},
    message => sub{message_git_changes_pushed(@_)},
    remedy  => sub{remedy_git_changes_pushed(@_, 0)}
  },
  {
    name    => 'Email DDS-Release-Announce list',
    verify  => sub{verify_email_list(@_)},
    message => sub{message_email_dds_release_announce(@_)},
    remedy  => sub{remedy_email_dds_release_announce(@_)}
  },
);

# For all steps, check for missing required attributes, fill others
foreach my $step_index (1..scalar(@release_steps)) {
  my $step = $release_steps[$step_index - 1];
  my @required = (
    'name',
    'verify',
    'message',
  );
  foreach my $attr (@required) {
    if (not exists $step->{$attr}) {
      die "Step #${step_index} is missing required \"${attr}\" attribute!";
    }
  }
  if ($step->{prereqs}) {
    foreach my $prereq_name (@{$step->{prereqs}}) {
      get_step_by_name($prereq_name);
    }
  }
  $step->{verified} = 0;
  if (not exists $step->{can_force}) {
    $step->{can_force} = 0;
  }
}

sub get_step_by_name {
  my ($name) = @_;
  foreach my $i (1..scalar(@release_steps)) {
    my $step = $release_steps[$i-1];
    if ($name eq $step->{name}) {
      return $i;
    }
  }
  die "Could not find step named: $name";
}

my $half_divider = '-' x 40;
my $divider = $half_divider x 2;

sub run_step {
  my ($settings, $release_steps, $step_count) = @_;
  my $step = $release_steps->[$step_count-1];
  my $title = $step->{name};

  return if $step->{skip} || $step->{verified};
  print "$step_count: $title\n";
  return if $settings->{list};

  # Check Prerequisite Steps
  if ($step->{prereqs}) {
    foreach my $prereq_name (@{$step->{prereqs}}) {
      run_step($settings, $release_steps, get_step_by_name($prereq_name));
    }
  }

  # Run the verification
  if (!$step->{verify}($settings)) {
    # Failed
    print "$divider\n";
    print "  " . $step->{message}($settings) . "\n";

    my $remedied = 0;
    my $forced  = 0;
    # If a remedy is available and --remedy specified
    if ($step->{remedy} && $settings->{remedy}) {
      # Try remediation
      if (!$step->{remedy}($settings)) {
        print "  !!!! Remediation did not complete\n";
      # Reverify
      } elsif (!$step->{verify}($settings)) {
        print "  !!!! Remediation did not pass verification\n";
      } else {
        $remedied = 1;
      }
    }

    # If not remedied, try forcing
    if (!$remedied && $step->{can_force} && $settings->{force}) {
      $forced = 1;
      print "Forcing Past This Step!\n";
    }

    if (!($remedied || $forced)) {
      if ($step->{remedy} && !$settings->{remedy}) {
        print "  Use --remedy to attempt a fix\n";
      }
      if ($step->{force} && !$settings->{force}) {
        print "  Use --force to force past this step\n";
      }
    }

    die unless ($remedied || $forced);
    print "$divider\n";
  }

  $step->{verified} = 1;
}

if (!$print_help && ($global_settings{list} || %parsed_version)) {
  my @steps_to_do;
  my $no_steps = scalar(@release_steps);
  if (defined($ARGV[1]) && !is_option($ARGV[2])) {
    @steps_to_do = parse_step_expr($no_steps, $ARGV[2]);
  } else {
    @steps_to_do = 1..$no_steps;
  }

  foreach my $step_count (@steps_to_do) {
    run_step(\%global_settings, \@release_steps, $step_count);
  }
} elsif (!$print_help) {
  $status = 1;
  $print_help = 1;
  print "Invalid Arguments, see --help for Valid Usage\n";
} else {
  print usage();
}

exit $status;
