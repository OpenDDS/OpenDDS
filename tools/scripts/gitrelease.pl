#!/usr/bin/env perl

use strict;
use warnings;

use POSIX;
use Cwd;
use File::Basename;
use File::stat;
use File::Temp qw/tempdir/;
use File::Path qw/make_path/;
use Getopt::Long;
use JSON::PP;
use Storable qw/dclone/;

use LWP::UserAgent;
use Net::FTP::File;
use Time::Piece;
use Pithub;

use FindBin;
use lib "$FindBin::RealBin/modules";
use ConvertFiles;
use version_utils;
use command_utils;
use ChangeDir;

my $zero_version = parse_version("0.0.0");

my $base_name_prefix = "OpenDDS-";
my $default_github_user = "objectcomputing";
my $default_remote = "origin";
my $default_branch = "master";
my $release_branch_prefix = "branch-";
my $repo_name = "OpenDDS";
my $oci_download = "https://download.objectcomputing.com";
my $default_download_url = "$oci_download/OpenDDS";
my $ace_tao_filename = "ACE+TAO-2.2a_with_latest_patches_NO_makefiles.tar.gz";
my $ace_tao_url = "$oci_download/TAO-2.2a/$ace_tao_filename";
my $ace_root = "ACE_wrappers";
my $git_name_prefix = "DDS-";
my $default_post_release_metadata = "dev";
my $workspace_info_filename = "info.json";

$ENV{TZ} = "UTC";
Time::Piece::_tzset;
my $timefmt = "%a %b %d %H:%M:%S %Z %Y";

my $release_timestamp_fmt = "%b %e %Y";

# Number of lines after start of file to insert news_template
my $news_header_lines = 2;

my $news_template = "## Version %s of OpenDDS\n" . <<"ENDOUT";
%s

### Additions:
- TODO: Add your features here

### Fixes:
- TODO: Add your fixes here

### Notes:
- TODO: Add your notes here

ENDOUT

sub get_news_post_release_msg {
  my $settings = shift();
  my $ver = $settings->{parsed_next_version}->{release_string};
  return "OpenDDS $ver is currently in development, so this list might change.";
}
my $news_post_release_msg_re =
  qr/^OpenDDS .* is currently in development, so this list might change\.$/;

sub get_news_release_msg {
  my $settings = shift();
  return "OpenDDS $settings->{version} was released on $settings->{timestamp}.";
}

my $insert_news_template_after_line = 1;
sub insert_news_template($$) {
  my $settings = shift();
  my $post_release = shift();

  my $version = ($post_release ?
    $settings->{parsed_next_version} : $settings->{parsed_version})->{release_string};
  my $release_msg = $post_release ?
    get_news_post_release_msg($settings) : get_news_release_msg($settings);

  # Read News File
  open my $news_file, '<', "NEWS.md";
  my @lines = <$news_file>;
  close $news_file;

  # Insert Template
  my @new_lines = ();
  push(@new_lines, @lines[0..$insert_news_template_after_line]);
  push(@new_lines, sprintf($news_template, $version, $release_msg));
  push(@new_lines, @lines[$insert_news_template_after_line+1..scalar(@lines)-1]);

  # Write News File with Template
  open $news_file, '>', "NEWS.md";
  foreach my $line (@new_lines) {
    print $news_file $line;
  }
  close $news_file;
}

sub get_version_line {
  my $settings = shift();
  my $post_release = shift() // 0;
  my $line = "This is OpenDDS version ";
  if ($post_release) {
    $line .= "$settings->{next_version} (NOT A RELEASE)";
  }
  else {
    $line .= "$settings->{version}, released $settings->{timestamp}";
  }
  return $line;
}

sub usage {
  return
    "gitrelease.pl WORKSPACE VERSION [options]\n" .
    "gitrelease.pl --help | -h\n" .
    "gitrelease.pl --list-all\n";
}

sub arg_error {
  print STDERR "ERROR: " . shift() . "\n" .
    "Usage:\n" .
    usage() .
    "See --help for more information\n";
  exit(1);
}

sub help {
  return usage() .
    "\n" .
    "Positional Arguments:\n" .
    "    WORKSPACE:           Path of intermediate files directory. If it doesn't\n" .
    "                         exist, it will be created. This should a new one for\n" .
    "                         each release.\n" .
    "    VERSION:             Release version in a.b or a.b.c notation\n" .
    "\n" .
    "Options:\n" .
    "  --help | -h            Print this message\n" .
    "  --list                 Just show step names that would run given the current\n" .
    "                         options. (default perform check)\n" .
    "  --list-all             Same as --list, but show every step regardless of\n" .
    "                         options.\n" .
    "  --steps STEPS          Optional, Steps to perform, default is all\n" .
    "                         See \"Step Expressions\" Below for what it accepts.\n" .
    "  --remedy               Remediate problems where possible\n" .
    "  --force                Keep going if possible\n" .
    "  --remote=NAME          Valid git remote for OpenDDS (default: ${default_remote})\n" .
    "  --branch=NAME          Valid git branch for cloning (default: ${default_branch})\n" .
    "  --github-user=NAME     User or organization name for github updates\n" .
    "                         (default: ${default_github_user})\n" .
    "  --download-url=URL     URL to verify FTP uploads\n" .
    "                         (default: ${default_download_url})\n" .
    "  --mock                 Enable mock release-specific checks and fake the\n" .
    "                         Doxygen and Devguide steps if they're not being\n" .
    "                         skipped.\n" .
    "  --mock-with-doxygen    Same as --mock, but with real Doxygen steps if they're\n" .
    "                         not being skipped\n" .
    "  --micro                Do a patch/micro level release. Requires --branch.\n" .
    "                         (default is no)\n" .
    "                         The difference from a regular release is that anything\n" .
    "                         to do with the the devguide, the doxygen, the website,\n" .
    "                         or creating or updating a release branch is skipped.\n" .
    "                         Run gitrelease.pl --list --micro to see the exact steps.\n" .
    "  --next-version=VERSION What to set the verion to after release. Must be the\n" .
    "                         same format as the VERSION positional argument.\n" .
    "                         (default is to increment the minor version field).\n" .
    "  --metadata=ID          What to append to the post-release version string like\n" .
    "                         X.Y.Z-ID (default: ${default_post_release_metadata})\n" .
    "  --skip-devguide        Skip getting and including the devguide\n" .
    "  --skip-doxygen         Skip getting ACE/TAO and generating and including the\n" .
    "                         doxygen docs\n" .
    "  --skip-website         Skip updating the website\n" .
    "  --skip-ftp             Skip the FTP upload\n" .
    "  --ignore-github-token  Don't require GITHUB_TOKEN\n" .
    "  --ftp-active           Use an active FTP connection. (default is passive)\n" .
    "\n" .
    "Environment Variables:\n" .
    "  GITHUB_TOKEN           GitHub token with repo access to publish release on\n" .
    "                         GitHub.\n" .
    "  FTP_USERNAME           FTP Username\n" .
    "  FTP_PASSWD             FTP Password\n" .
    "  FTP_HOST               FTP Server Address\n" .
    "\n" .
    "Step Expressions\n" .
    "  The STEPS argument accepts a specific notation for what steps to run. They\n" .
    "  work like a bitmap, with subexpressions enabling or disabling steps. Later\n" .
    "  subexpressions can overrule earlier ones. The initial list of steps are all\n" .
    "  disabled at the beginning unless the first subexpression is negative (starts\n" .
    "  with ^), then it starts with all steps enabled.\n" .
    "\n" .
    "  If that doesn't make sense, here are some examples:\n" .
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
    "\n" .
    "  Finally you can combine subexpressions by delimiting them with commas:\n" .
    "    1,2,3\n" .
    "      Do steps 1, 2, and 3\n" .
    "    ^5-,10\n" .
    "      Don't do step 5 or any step after that, except for 10\n" .
    "    3-20,^5-7,^14\n" .
    "      Do steps 3 through 20, except for steps 5, 6, 7, and 14\n";
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
    }
    elsif (/^##[^#]/ && $saw_version) { # Until we come to the next h2
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
    news_contents_excerpt($settings->{version});

  return $result;
}

sub run_command {
  return !command_utils::run_command(@_,
    script_name => 'gitrelease.pl',
  );
}

sub yes_no {
  my $message = shift;
  print("$message [y/n] ");
  while (<STDIN>) {
    chomp;
    if ($_ eq "n") {
      return 0;
    }
    elsif ($_ eq "y") {
      return 1;
    }
    else {
      print "Please answer y or n. ";
    }
  }
}

sub touch_file {
  my $path = shift();

  my $dir = dirname($path);
  if (not -d $dir) {
    make_path($dir) or die "Couldn't make directory $dir for touch_file: $!\n";
  }

  if (not -f $path) {
    open(my $fh, '>', $path) or die "Couldn't open file $path: $!";
  }
  else {
    my $t = time();
    utime($t, $t, $path) || die "Couldn't touch file $path: $!";
  }
}

my %dummy_release_files = ();

sub create_dummy_release_file {
  my $path = shift();

  my $filename = basename($path);
  print "Creating dummy $filename because of mock release\n";
  $dummy_release_files{$filename} = 1;
  open(my $fh, '>', $path) or die "Couldn't open file $path: $!";
  print $fh "This is a dummy file because the release was mocked\n";
  close($fh);
}

sub new_pithub {
  my $settings = shift();

  return undef if ($settings->{ignore_github_token});

  if (!defined($settings->{github_token})) {
    die("GITHUB_TOKEN must be defined");
  }

  return Pithub->new(
    user => $settings->{github_user},
    repo => $settings->{github_repo},
    token => $settings->{github_token},
  );
}

sub read_json_file {
  my $path = shift();

  open my $f, '<', $path or die("Can't open JSON file $path: $!");
  local $/;
  my $text = <$f>;
  close $f;

  return decode_json($text);
}

sub write_json_file {
  my $path = shift();
  my $what = shift();

  open my $f, '>', $path or die("Can't open JSON file $path: $!");
  # Writes indented, space-separated JSON in a reproducible order
  print $f JSON::PP->new->pretty->canonical->encode($what);
  close $f;
}

sub expand_releases {
  my $releases = shift();
  for my $release (@{$releases}) {
    $release->{version} = parse_version($release->{version});
  }
  return $releases;
}

sub compress_releases {
  my $releases = shift();

  # Make a copy and replace the parsed version hashes with the version strings
  my @copy = @{dclone($releases)};
  for my $release (@copy) {
    $release->{version} = $release->{version}->{string};
  }
  return \@copy;
}

sub write_releases {
  my $path = shift();
  my $releases_ref = shift();

  write_json_file($path, compress_releases($releases_ref));
}

sub write_workspace_info {
  my $settings = shift();

  write_json_file($settings->{workspace_info_file_path}, {
    mock => $settings->{mock},
    micro => $settings->{micro},
    version => $settings->{version},
    next_version => $settings->{next_version},
    is_highest_version => $settings->{is_highest_version},
    release_occurred => $settings->{release_occurred},
  });
}

sub compare_workspace_info {
  my $settings = shift();
  my $info = shift();
  my $key = shift();

  if ($settings->{$key} ne $info->{$key}) {
    print STDERR "Workspace has \"$info->{$key}\" for $key, but we expected \"$settings->{$key}\"\n";
    return 1;
  }
  return 0;
}

sub check_workspace {
  my $settings = shift();

  return if $settings->{list};

  if (!-d $settings->{workspace}) {
    print("Creating workspace directory \"$settings->{workspace}\"\n");
    if (!make_path($settings->{workspace})) {
      die("Failed to create workspace: \"$settings->{workspace}\"");
    }
  }

  if (-f $settings->{workspace_info_file_path}) {
    my $info = read_json_file($settings->{workspace_info_file_path});

    my $invalid = 0;
    $invalid |= compare_workspace_info($settings, $info, 'mock');
    $invalid |= compare_workspace_info($settings, $info, 'micro');
    $invalid |= compare_workspace_info($settings, $info, 'version');
    $invalid |= compare_workspace_info($settings, $info, 'next_version');
    if ($invalid) {
      die("Inconsistent with existing workspace, see above for details");
    }

    $settings->{is_highest_version} = $info->{is_highest_version};
    if ($info->{release_occurred}) {
      print("Release occurred flag set, assuming $info->{version} was released!\n");
    }
    $settings->{release_occurred} = $info->{release_occurred};
  }
  else {
    my $previous_releases = get_releases($settings);
    $settings->{is_highest_version} = scalar(@{$previous_releases}) == 0 ||
      version_greater_equal($settings->{parsed_version},
        $previous_releases->[0]->{version});
    $settings->{release_occurred} = 0;
    write_workspace_info($settings);
  }
}

sub download {
  my $url = shift();
  my %args = @_;

  my $agent = LWP::UserAgent->new();

  my %get_args = ();
  my $to = "";
  if (exists($args{save_path})) {
    $get_args{':content_file'} = $args{save_path};
    $to = " to \"$args{save_path}\"";
  }

  print("Downloading $url$to...\n");
  my $response = $agent->get($url, %get_args);
  if (!$response->is_success()) {
    print STDERR "ERROR: ", $response->status_line(), "\n";
    return 0;
  }

  if (exists($args{content_ref})) {
    ${$args{content_ref}} = $response->decoded_content();
  }

  return 1;
}

sub get_current_branch {
  open(my $fh, "git rev-parse --abbrev-ref HEAD|") or die "git: $!";
  my $branch_name;
  for my $line (<$fh>) {
    chomp($line);
    if ($line =~ /^\S+$/) {
      $branch_name = $line;
    }
  }
  close($fh);
  die("Couldn't get branch name") if (!$branch_name);
  return $branch_name;
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
    }
    else {
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
    }
    elsif ($range) {
      $y = $no_steps;
    }
    else {
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

sub parse_release_tag {
  my $tag = shift();

  my $parsed;
  if ($tag =~ /^DDS-(.*)$/) {
    $parsed = parse_version($1) if $1;
    $parsed->{tag_name} = $tag if $parsed;
  }
  if (!$parsed) {
    die("Invalid release tag name: $tag");
  }
  return $parsed;
}

sub get_releases {
  my $settings = shift();

  if (!defined($settings->{pithub})) {
    return [{
      version => $zero_version,
      published_at => '2001-09-09T01:46:40',
      html_url => 'https://opendds.org',
      assets => [],
    }];
  }

  my $release_list = $settings->{pithub}->repos->releases->list();
  unless ($release_list->success) {
    die("error accessing github: $release_list->response->status_line");
  }

  my @releases = ();
  while (my $release = $release_list->next) {
    next if $release->{prerelease};
    my $parsed = parse_release_tag($release->{tag_name});
    my @assets = ();
    for my $asset (@{$release->{assets}}) {
      if ($asset->{name} !~ /tar\.gz|zip$/) {
        next;
      }
      push(@assets, {
        name => $asset->{name},
        browser_download_url => $asset->{browser_download_url},
      });
    }
    # Sort by reverse name to put zip files first for the website
    @assets = sort { $b->{name} cmp $a->{name} } @assets;
    push(@releases, {
      version => $parsed,
      published_at => $release->{published_at},
      html_url => $release->{html_url},
      assets => \@assets,
    });
  }

  my @sorted = sort { version_cmp($b->{version}, $a->{version}) } @releases;
  return \@sorted;
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
  }
  else {
    $settings->{alt_url} = $url;
  }
  return $result;
}

sub message_git_remote {
  my $settings = shift;
  return "Remote $settings->{remote} in git repo has url $settings->{alt_url}\n" .
    "  this is different from what the script is expecting: $settings->{git_url}\n" .
    "  This must be resolved by either making sure the local git repo has the\n" .
    "  correct remote, or by changing what the script is expecting by using the\n" .
    "  --remote and --github-user options.";
}

############################################################################

sub verify_git_status_clean {
  my $settings = shift();
  my $step_options = shift() // {};
  my $strict = $step_options->{strict} // 0;
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
  my $post_release = shift() // 0;
  my $changelog = shift() // 1;
  my $version = $settings->{version};

  if (!run_command("git --no-pager diff HEAD")) {
    return 0;
  }
  return 0 if (!yes_no("Would you like to add and commit these changes?"));
  if (!$post_release && $changelog) {
    system("git add docs/history/ChangeLog-$version") == 0
      or die "Could not execute: git add docs/history/ChangeLog-$version";
  }
  system("git add -u") == 0 or die "Could not execute: git add -u";
  my $message;
  if ($post_release) {
    $message = "OpenDDS Post Release $version";
  }
  else {
    $message = "OpenDDS Release $version";
  }
  system("git commit -m '" . $message . "'") == 0 or die "Could not execute: git commit -m";
  return 1;
}

############################################################################

sub verify_update_version_file {
  my $settings = shift();
  my $step_options = shift() // {};
  my $post_release = $step_options->{post_release} // 0;

  my $correct = 0;
  my $line = quotemeta(get_version_line($settings, $post_release));

  open(VERSION, 'VERSION.txt') or die "Opening: $!";
  while (<VERSION>) {
    if ($_ =~ /$line/) {
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
  my $post_release = shift() // 0;

  print "  >> Updating VERSION.txt file\n";

  my $corrected = 0;
  my $line = get_version_line($settings, $post_release);

  open(VERSION, "+< VERSION.txt") or die "Opening: $!";
  my $out = "";
  while (<VERSION>) {
    if (!$corrected && s/^This is OpenDDS version .*$/$line/) {
      $corrected = 1;
    }
    $out .= $_;
  }

  seek(VERSION, 0, 0) or die "Seeking: $!";
  print VERSION $out or die "Printing: $!";
  truncate(VERSION, tell(VERSION)) or die "Truncating: $!";
  close(VERSION) or die "Closing: $!";

  return $corrected;
}

############################################################################
sub find_previous_tag {
  my $settings = shift();
  my $remote = $settings->{remote};
  my $release_version = $settings->{parsed_version};
  my $prev_version_tag = "";
  my $prev_version = $zero_version;

  open(GITTAG, "git tag --list |") or die "Opening $!";
  while (<GITTAG>) {
    chomp;
    next unless /^$git_name_prefix([\d\.]*)$/;
    my $version = $1;
    my $tag_version = parse_version($version);
    # If this is less than the release version, but the largest seen yet
    if (version_lesser($tag_version, $release_version) &&
        version_greater($tag_version, $prev_version)) {
      $prev_version_tag = $_;
      $prev_version = $tag_version;
    }
  }
  close(GITTAG);
  return $prev_version_tag;
}

sub verify_changelog {
  my $settings = shift();
  my $status = open(CHANGELOG, $settings->{changelog});
  if ($status) {
    close(CHANGELOG);
  }
  return $status;
}

sub message_changelog {
  my $settings = shift();
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
    }
    elsif (length($next_line) < 65) {
      $result .= "          $next_line\n";
    }
    else {
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
    }
    elsif (/^Merge: *(.*)/) {
      # Ignore
    }
    elsif (/^Author: *(.*)/) {
      $author = $1;
      $author =~ s/</ </;
    }
    elsif (/^Date: *([0-9]+)/) {
      $date = POSIX::strftime($timefmt, gmtime($1));
    }
    elsif (/^ +(.*) */) {
      $comment .= "$1\n";
    }
    elsif (/^[AMD]\s+(.*) *$/) {
      $file_mod_list .= "        * $1:\n";
    }
    elsif (/^[CR][0-9]*\s+(.*) *$/) {
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

  # If doing a mock release, it's okay for the changelog to be empty.
  return $settings->{mock} ? 1 : $changed;
}
############################################################################

# This section deals with the AUTHORS file
# See $DDS_ROOT/.mailmap file for how to add individual corrections

my $github_email_re = qr/^(?:\d+\+)?(.*)\@users.noreply.github.com$/;

my @global_authors = ();

sub search_authors {
  my @authors = @{shift()};
  my $email = shift() // 0;
  my $name = shift() // 0;

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

  my $tmp_authors_path = "$settings->{workspace}/temp_authors";
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
  }
  else {
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

  return $has_version;
}

sub message_news_file_section {
  my $settings = shift();
  my $version = $settings->{version};
  return "NEWS.md file release $version section needs updating";
}

sub remedy_news_file_section {
  my $settings = shift();
  my $version = $settings->{version};
  print "  >> Adding $version section template to NEWS.md\n";
  # If doing a mock release, the template is okay.
  my $ok = $settings->{mock};
  if (!$ok) {
    print "  !! Manual update to NEWS.md needed\n";
  }
  insert_news_template($settings, 0);
  return $ok;
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
  my $real_release = !$settings->{mock};
  while (<NEWS>) {
    if ($_ =~ /^## Version $metaversion of OpenDDS/) {
      $has_version = 1;
    }
    elsif ($real_release && $_ =~ /TODO: Add your features here/) {
      $corrected_features = 0;
    }
    elsif ($real_release && $_ =~ /TODO: Add your fixes here/) {
      $corrected_fixes = 0;
    }
  }
  close(NEWS);

  return $has_version && $corrected_features && $corrected_fixes;
}

sub message_update_news_file {
  return "NEWS.md file needs updating with current version release notes";
}

############################################################################

sub verify_news_timestamp {
  my $settings = shift();
  my $has_post_release_msg = 0;
  open(my $news_file, 'NEWS.md');
  while (<$news_file>) {
    if ($_ =~ $news_post_release_msg_re) {
      $has_post_release_msg = 1;
      last;
    }
  }
  close($news_file);
  return !$has_post_release_msg;
}

sub message_news_timestamp {
  return "The NEWS.md section for this release needs its timestamp inserted";
}

sub remedy_news_timestamp {
  my $settings = shift();
  my $release_msg = get_news_release_msg($settings);

  # Read News File
  open(my $news_file, '<', "NEWS.md");
  my @lines = <$news_file>;
  close $news_file;

  # Insert Template
  foreach my $line (@lines) {
    last if $line =~ s/$news_post_release_msg_re/$release_msg/;
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
  my $step_options = shift() // 0;
  my $post_release = $step_options->{post_release} // 0;

  if (!$post_release && verify_update_version_h_file($settings, {post_release => 1})) {
    die "ERROR: Version.h already indicates that the post release was done!";
  }

  my $parsed_version = $post_release ?
    $settings->{parsed_next_version} : $settings->{parsed_version};
  my $metaversion = quotemeta($parsed_version->{string});
  my $release = $post_release ? "0" : "1";
  my $metadata = quotemeta($parsed_version->{metadata});

  my $matched_major = 0;
  my $matched_minor = 0;
  my $matched_micro = 0;
  my $matched_metadata = 0;
  my $matched_release = 0;
  my $matched_version = 0;

  my $status = open(VERSION_H, 'dds/Version.h') or die("Opening: $!");
  while (<VERSION_H>) {
    if ($_ =~ /^#define OPENDDS_MAJOR_VERSION $parsed_version->{major}$/) {
      ++$matched_major;
    }
    elsif ($_ =~ /^#define OPENDDS_MINOR_VERSION $parsed_version->{minor}$/) {
      ++$matched_minor;
    }
    elsif ($_ =~ /^#define OPENDDS_MICRO_VERSION $parsed_version->{micro}$/) {
      ++$matched_micro;
    }
    elsif ($_ =~ /^#define OPENDDS_VERSION_METADATA "$metadata"$/) {
      ++$matched_metadata;
    }
    elsif ($_ =~ /^#define OPENDDS_IS_RELEASE $release$/) {
      ++$matched_release;
    }
    elsif ($_ =~ /^#define OPENDDS_VERSION "$metaversion"$/) {
      ++$matched_version;
    }
  }
  close(VERSION_H);

  return
    $matched_major == 1 &&
    $matched_minor == 1 &&
    $matched_micro == 1 &&
    $matched_metadata == 1 &&
    $matched_release == 1 &&
    $matched_version == 1;
}

sub message_update_version_h_file {
  return "dds/Version.h file needs updating with current version"
}

sub remedy_update_version_h_file {
  my $settings = shift();
  my $post_release = shift() // 0;

  my $parsed_version = $post_release ?
    $settings->{parsed_next_version} : $settings->{parsed_version};
  my $version = $parsed_version->{string};
  my $release = $post_release ? "0" : "1";

  print "  >> Updating dds/Version.h file for $version\n";

  my $corrected_major = 0;
  my $corrected_minor = 0;
  my $corrected_micro = 0;
  my $corrected_metadata = 0;
  my $corrected_release = 0;
  my $corrected_version = 0;

  my $major_line = "#define OPENDDS_MAJOR_VERSION $parsed_version->{major}";
  my $minor_line = "#define OPENDDS_MINOR_VERSION $parsed_version->{minor}";
  my $micro_line = "#define OPENDDS_MICRO_VERSION $parsed_version->{micro}";
  my $metadata_line = "#define OPENDDS_VERSION_METADATA \"$parsed_version->{metadata}\"";
  my $release_line = "#define OPENDDS_IS_RELEASE $release";
  my $version_line = "#define OPENDDS_VERSION \"$version\"";

  open(VERSION_H, "+< dds/Version.h") or die "Opening: $!";

  my $out = "";

  while (<VERSION_H>) {
    if (s/^#define OPENDDS_MAJOR_VERSION .*$/$major_line/) {
      ++$corrected_major;
    }
    elsif (s/^#define OPENDDS_MINOR_VERSION .*$/$minor_line/) {
      ++$corrected_minor;
    }
    elsif (s/^#define OPENDDS_MICRO_VERSION .*$/$micro_line/) {
      ++$corrected_micro;
    }
    elsif (s/^#define OPENDDS_VERSION_METADATA .*$/$metadata_line/) {
      ++$corrected_metadata;
    }
    elsif (s/^#define OPENDDS_IS_RELEASE .*$/$release_line/) {
      ++$corrected_release;
    }
    elsif (s/^#define OPENDDS_VERSION .*$/$version_line/) {
      ++$corrected_version;
    }
    $out .= $_;
  }

  seek(VERSION_H, 0, 0) or die "Seeking: $!";
  print VERSION_H $out or die "Printing: $!";
  truncate(VERSION_H, tell(VERSION_H)) or die "Truncating: $!";
  close(VERSION_H) or die "Closing: $!";

  return
    $corrected_major == 1 &&
    $corrected_minor == 1 &&
    $corrected_micro == 1 &&
    $corrected_metadata == 1 &&
    $corrected_release == 1 &&
    $corrected_version == 1;
}

############################################################################

sub verify_update_prf_file {
  my $settings = shift();
  my $step_options = shift() // {};
  my $post_release = $step_options->{post_release} // 0;

  my $line = quotemeta(get_version_line($settings, $post_release));
  my $metaversion = quotemeta($post_release ?
    $settings->{next_version} : $settings->{version});
  my $matched_header = 0;
  my $matched_version = 0;

  my $status = open(PRF, 'PROBLEM-REPORT-FORM') or die("Opening: $!");
  while (<PRF>) {
    if ($_ =~ /^$line/) {
      ++$matched_header;
    }
    elsif ($_ =~ /OpenDDS VERSION: $metaversion$/) {
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
  my $post_release = shift() // 0;

  my $version = $post_release ?
    $settings->{next_version} : $settings->{version};
  print "  >> Updating PROBLEM-REPORT-FORM file for $version\n";

  my $corrected_header  = 0;
  my $corrected_version = 0;
  my $header_line = get_version_line($settings, $post_release);
  my $version_line = "OpenDDS VERSION: $version";

  my $out = "";
  open(PRF, '+< PROBLEM-REPORT-FORM') or die("Opening $!");
  while (<PRF>) {
    if (s/^This is OpenDDS version .*$/$header_line/) {
      ++$corrected_header;
    }
    elsif (s/OpenDDS VERSION: .*$/$version_line/) {
      ++$corrected_version;
    }
    $out .= $_;
  }

  seek(PRF, 0, 0) or die("Seeking: $!");
  print PRF $out or die("Printing: $!");
  truncate(PRF, tell(PRF)) or die("Truncating: $!");
  close(PRF) or die("Closing: $!");

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
  my $push_latest_release = shift() // $settings->{is_highest_version};
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
    if (/^$settings->{git_tag}$/) {
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
  }
  else {
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
  }
  else {
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

sub checksum_common {
  my $type = shift();
  my $verify = shift();
  my $settings = shift();
  my $step_options = shift() // {};

  my $chdir = ChangeDir->new($settings->{workspace});
  my $checksum_file = $settings->{"${type}_src"};

  my @cmd = ("${type}sum");
  my $capture = {};
  if ($verify) {
    return 0 unless (-f $checksum_file);
    push(@cmd, '--check', $step_options->{remedy_verify} ? '--quiet' : '--status', $checksum_file);
  }
  else {
    push(@cmd, get_source_release_files($settings));
    $capture->{stdout} = $checksum_file;
  }

  return run_command(\@cmd, capture => $capture);
}

sub verify_md5_checksum {
  return checksum_common('md5', 1, @_);
}

sub message_md5_checksum {
  return "Generate the MD5 checksum file";
}

sub remedy_md5_checksum {
  return checksum_common('md5', 0, @_);
}

############################################################################

sub verify_sha256_checksum {
  return checksum_common('sha256', 1, @_);
}

sub message_sha256_checksum {
  return "Generate the sha256 checksum file";
}

sub remedy_sha256_checksum {
  return checksum_common('sha256', 0, @_);
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

  if ($settings->{mock}) {
    print "Touch $archive because of mock release\n";
    touch_file($archive);
    return 1;
  }

  return download($ace_tao_url, save_path => $archive);
}

############################################################################

sub ace_tao_sanity_file {
  my $settings = shift();

  return "$settings->{ace_root}/ace/ACE.h";
}

sub verify_extract_ace_tao {
  my $settings = shift();

  return -f ace_tao_sanity_file($settings);
}

sub message_extract_ace_tao {
  return "Extract OCI ACE/TAO";
}

sub remedy_extract_ace_tao {
  my $settings = shift();

  if ($settings->{dummy_doxygen}) {
    my $file = ace_tao_sanity_file($settings);
    print "Touch $file because of mock release\n";
    touch_file($file);
    return 1;
  }

  my $archive = "$settings->{workspace}/$ace_tao_filename";
  print "Extracting $archive...\n";
  return run_command("tar xzf $archive -C $settings->{workspace}");
}

############################################################################

sub doxygen_sanity_file {
  my $settings = shift();
  return "$settings->{clone_dir}/html/dds/index.html";
}

sub verify_gen_doxygen {
  my $settings = shift();
  return -f doxygen_sanity_file($settings);
}

sub message_gen_doxygen {
  my $settings = shift();
  return "Doxygen documentation needs generating in dir $settings->{clone_dir}\n";
}

sub remedy_gen_doxygen {
  my $settings = shift();

  if ($settings->{dummy_doxygen}) {
    my $file = doxygen_sanity_file($settings);
    print "Touch $file because of mock release\n";
    touch_file($file);
    return 1;
  }

  my $curdir = getcwd;
  $ENV{DDS_ROOT} = $settings->{clone_dir};
  $ENV{ACE_ROOT} = $settings->{ace_root};
  chdir($ENV{DDS_ROOT});
  my $result = run_command("$ENV{DDS_ROOT}/tools/scripts/generate_combined_doxygen.pl . -is_release");
  chdir($curdir);
  return $result;
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

  my $ver = "$settings->{workspace}/$settings->{devguide_ver}";
  my $lat = "$settings->{workspace}/$settings->{devguide_lat}";

  if ($settings->{mock}) {
    create_dummy_release_file($ver);
    create_dummy_release_file($lat);
    return 1;
  }

  my $devguide_url = "svn+ssh://svn.ociweb.com/devguide/opendds/trunk/$settings->{devguide_ver}";
  print "Downloading devguide\n$devguide_url\n";
  my $result = system("svn cat $devguide_url > $ver");
  $result = $result || system("svn cat $devguide_url > $lat");
  return !$result;
}

############################################################################

sub get_mime_type {
  my $filename = shift();

  # Offical IANA list: https://www.iana.org/assignments/media-types/media-types.xhtml
  if ($filename =~ /\.gz$/) {
    return 'application/gzip';
  }
  elsif ($filename =~ /\.zip$/) {
    return 'application/zip';
  }
  elsif ($filename =~ /\.pdf$/) {
    return 'application/pdf';
  }
  elsif ($filename =~ /\.(md5|sha256)$/) {
    return 'text/plain';
  }
  else {
    die("ERROR: can't determine the MIME type of ${filename}");
  }
}

sub get_source_release_files {
  my $settings = shift();

  return (
    $settings->{tgz_src},
    $settings->{zip_src},
  );
}

sub get_github_release_files {
  my $settings = shift();

  my @files = get_source_release_files($settings);

  push(@files, $settings->{md5_src}, $settings->{sha256_src});
  if (!$settings->{skip_devguide}) {
    push(@files, $settings->{devguide_ver});
  };

  return @files;
}

sub get_ftp_release_files {
  my $settings = shift();

  my @files = get_github_release_files($settings);

  if (!$settings->{skip_devguide}) {
    push(@files, $settings->{devguide_lat}) if ($settings->{is_highest_version});
  }
  if (!$settings->{skip_doxygen}) {
    push(@files, $settings->{tgz_dox});
    push(@files, $settings->{zip_dox});
  }

  return @files;
}

my $PRIOR_RELEASE_PATH = 'previous-releases/';

sub verify_ftp_upload {
  my $settings = shift();

  my $url = "$settings->{download_url}/";
  if ($url !~ /^https?:\/\//) {
    $url = "http://$url";
  }
  $url .= $PRIOR_RELEASE_PATH if (!$settings->{is_highest_version});
  my $content;
  if (!download($url, content_ref => \$content)) {
    print "Could not get current release files from $url";
    return 0;
  }

  foreach my $file (get_ftp_release_files($settings)) {
    if ($content !~ /$file/) {
      print "$file not found in $url\n";
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

  # login to ftp server and setup binary file transfers
  my $ftp = Net::FTP->new(
    $settings->{ftp_host},
    Debug => 0,
    Port => $settings->{ftp_port},
    Passive => !$settings->{ftp_active},
  ) or die "Cannot connect to $settings->{ftp_host}: $@";
  $ftp->login($settings->{ftp_user}, $settings->{ftp_password})
    or die "Cannot login ", $ftp->message();
  $ftp->cwd($FTP_DIR)
    or die "Cannot change dir to $FTP_DIR ", $ftp->message();
  my @current_release_files = $ftp->ls()
    or die "Cannot ls() $FTP_DIR ", $ftp->message();
  $ftp->binary();

  my @new_release_files = get_ftp_release_files($settings);

  if ($settings->{is_highest_version}) {
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
      }
      else {
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
            $ftp->rename($file, $new_path)
              or die "Could not rename $file to $new_path: " . $ftp->message();
          }
          else {
            print "deleting $file\n";
            $ftp->delete($file) or die "Could not delete $file: " . $ftp->message();
          }
          last;
        }
      }
    }
  }
  else {
    print "Not highest version release, cd-ing to $PRIOR_RELEASE_PATH\n";
    $ftp->cwd($PRIOR_RELEASE_PATH)
      or die "Cannot change dir to $PRIOR_RELEASE_PATH ", $ftp->message();
  }

  # upload new release files
  foreach my $file (@new_release_files) {
    print "uploading $file\n";
    my $local_file = join("/", $settings->{workspace}, $file);
    $ftp->put($local_file, $file) or die "Failed to upload $file: " . $ftp->message();
  }

  $ftp->quit();
  return 1;
}

############################################################################
sub verify_github_upload {
  my $settings = shift();
  my $verified = 0;

  my $release_list = $settings->{pithub}->repos->releases->list();
  unless ($release_list->success) {
    printf "error accessing github: %s\n", $release_list->response->status_line;
  }
  else {
    while (my $row = $release_list->next) {
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

  # Try to do as much as possible before creating the release. If there's a
  # fatal issue while uploading the release assets, then the release has to be
  # manually deleted on GitHub before this step can be run again.
  my @assets = get_github_release_files($settings);
  my %asset_details;
  for my $filename (@assets) {
    open(my $fh, "$settings->{workspace}/$filename") or die("Can't open \"$filename\": $?");
    binmode($fh);
    my $size = stat($fh)->size;
    my $data;
    if ($size == 0 && !exists($dummy_release_files{$filename})) {
      die("$filename is empty and is not supposed to be!");
    }
    else {
      read $fh, $data, $size or die("Can't read \"$filename\": $?");
    }
    close($fh);

    $asset_details{$filename} = {
      content_type => get_mime_type($filename),
      data => $data,
    };
  }

  my $releases = $settings->{pithub}->repos->releases;
  my $text =
    "**Download $settings->{zip_src} (Windows) or $settings->{tgz_src} (Linux/macOS) " .
      "instead of \"Source code (zip)\" or \"Source code (tar.gz)\".**\n\n" .
    news_contents_excerpt($settings->{version});
  my $release = $releases->create(
    data => {
      name => "OpenDDS $settings->{version}",
      tag_name => $settings->{git_tag},
      body => $text,
    },
  );
  unless ($release->success) {
    print STDERR "error accessing github: $release->response->status_line\n";
    $rc = 0;
  }
  else {
    my $fail_msg = "\nThe release on GitHub has to be deleted manually before trying to verify\n" .
      "or remedy this step again";
    for my $filename (@assets) {
      print("Upload $filename\n");
      my $asset;
      my $asset_detail = $asset_details{$filename};
      eval {
        $asset = $releases->assets->create(
          release_id => $release->content->{id},
          name => $filename,
          content_type => $asset_detail->{content_type},
          data => $asset_detail->{data}
        );
      };
      if ($@) {
        die("Issue with \$releases->assets->create:", $@, $fail_msg);
      }
      unless ($asset->success) {
        print STDERR "error accessing github: $asset->response->status_line$fail_msg\n";
        $rc = 0;
      }
    }
  }
  return $rc;
}

############################################################################

my $website_releases_json = '_data/releases.json';

sub verify_website_release {
  # verify there are no differences between website-next-release branch and gh-pages branch
  my $settings = shift();
  my $remote = $settings->{remote};
  my $status;

  if ($status && !run_command("git add $website_releases_json")) {
    print STDERR "Couldn't git add!\n";
    $status = 0;
  }
  if ($status && !remedy_git_status_clean($settings, 0, 0)) {
    print STDERR "Couldn't commit release list!\n";
    $status = 0;
  }

  # fetch remote branches so we have up to date versions
  run_command("git fetch $remote website-next-release") or
    die("Couldn't fetch website-next-release!");
  run_command("git fetch $remote gh-pages") or
    die("Couldn't fetch gh-pages!");

  open(GITDIFF, "git diff $remote/website-next-release $remote/gh-pages|") or
    die("Couldn't run git diff");
  my $delta = "";
  while (<GITDIFF>) {
    if (/^...(.*)/) {
      $delta .= $_;
    }
  }
  close(GITDIFF);
  my $has_merged = length($delta) == 0;

  # See if the release we're doing is on the website
  open(my $git_show, "git show $remote/gh-pages:$website_releases_json|") or
    die("Couldn't run git show $website_releases_json");
  my $has_release = 0;
  for my $r (@{decode_json(do { local $/; <$git_show> })}) {
    if ($r->{version} eq $settings->{version}) {
      $has_release = 1;
      last;
    }
  }
  close($git_show);

  return $has_merged && $has_release;
}

sub message_website_release {
  my $settings = shift();
  my $remote = $settings->{remote};
  return "$remote/website-next-release branch needs to merge into\n" .
    "  $remote/gh-pages branch or release list needs to be updated";
}

sub remedy_website_release {
  my $settings = shift();
  print "Releasing gh-pages website\n";
  my $worktree = "$settings->{workspace}/temp_website";
  my $remote = $settings->{remote};
  my $rm_worktree = 0;
  my $status = 1;

  if (!-d $worktree && !run_command("git worktree add $worktree gh-pages")) {
    print STDERR
      "Couldn't create temporary website worktree!\n";
    $status = 0;
  }
  $rm_worktree = 1;
  my $prev_dir = getcwd;
  chdir $worktree;

  # Merge releases with the same major and minor versions, keeping one with the
  # highest micro version.
  my $releases = get_releases($settings);
  my $latest_ver = $releases->[0]->{version};
  my $major = $latest_ver->{major} + 1;
  my $minor = $latest_ver->{minor};
  for my $release (@{$releases}) {
    my $v = $release->{version};
    $release->{latest_in_series} =
      ($v->{major} != $major || $v->{minor} != $minor) ? 1 : 0;
    if ($release->{latest_in_series}) {
      $major = $v->{major};
      $minor = $v->{minor};
    }
  }
  write_releases($website_releases_json, $releases);
  if ($status && !run_command("git add $website_releases_json")) {
    print STDERR "Couldn't git add!\n";
    $status = 0;
  }
  if ($status && !remedy_git_status_clean($settings, 0, 0)) {
    print STDERR "Couldn't commit release list!\n";
    $status = 0;
  }

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
  print 'Email this text to announce the release' . "\n\n" .
    email_announce_contents($settings);
  return 1;
}

sub message_email_dds_release_announce {
  return 'Emails are needed to announce the release';
}

sub remedy_email_dds_release_announce {
  return 1;
}

############################################################################
sub verify_news_template_file_section {
  my $settings = shift();
  my $next_version = quotemeta($settings->{parsed_next_version}->{release_string});

  my $status = open(NEWS, 'NEWS.md');
  my $has_news_template = 0;
  while (<NEWS>) {
    if ($_ =~ /^## Version $next_version of OpenDDS/) {
      $has_news_template = 1;
      last;
    }
  }
  close(NEWS);

  return $has_news_template;
}

sub message_news_template_file_section {
  return "Template for next release in NEWS.md is missing";
}

sub remedy_news_template_file_section {
  my $settings = shift();
  insert_news_template($settings, 1);
  return 1;
}

############################################################################

sub verify_release_occurred_flag {
  my $settings = shift();

  return $settings->{release_occurred};
}

sub remedy_release_occurred_flag {
  my $settings = shift();

  $settings->{release_occurred} = 1;
  write_workspace_info($settings);

  return 1;
}

############################################################################

my $status = 0;
my $ignore_args = 0;

# Process Optional Arguments
my $print_help = 0;
my $print_list = 0;
my $print_list_all = 0;
my $step_expr = 0;
my $remedy = 0;
my $force = 0;
my $remote = $default_remote;
my $branch = $default_branch;
my $github_user = $default_github_user;
my $download_url = $default_download_url;
my $mock = 0;
my $mock_with_doxygen = 0;
my $micro = 0;
my $next_version = "";
my $metadata = $default_post_release_metadata;
my $skip_devguide = 0;
my $skip_doxygen = 0;
my $skip_website = 0;
my $skip_ftp = 0;
my $ignore_github_token = 0;
my $ftp_active = 0;
my $ftp_port = 0;

GetOptions(
  'help' => \$print_help,
  'list' => \$print_list,
  'list-all' => \$print_list_all,
  'steps=s' => \$step_expr,
  'remedy' => \$remedy,
  'force' => \$force,
  'remote=s' => \$remote,
  'branch=s' => \$branch,
  'github-user=s' => \$github_user,
  'download-url=s' => \$download_url,
  'mock' => \$mock,
  'mock-with-doxygen' => \$mock_with_doxygen,
  'micro' => \$micro,
  'next-version=s' => \$next_version,
  'metadata=s' => \$metadata,
  'skip-devguide' => \$skip_devguide,
  'skip-doxygen' => \$skip_doxygen ,
  'skip-website' => \$skip_website ,
  'skip-ftp' => \$skip_ftp,
  'ftp-active' => \$ftp_active,
  'ftp-port=s' => \$ftp_port,
  'ignore-github-token' => \$ignore_github_token,
) or arg_error("Invalid option");

if ($print_help) {
  print help();
  exit(0);
}

if ($print_list_all) {
  $print_list = 1;
  $ignore_args = 1;
}

$mock = 1 if ($mock_with_doxygen);

my $workspace = "";
my $parsed_version;
my $parsed_next_version;
my $version = "";
my $base_name = "";
my $release_branch = "";

if ($ignore_args) {
  $parsed_version = $zero_version;
  $parsed_next_version = $zero_version;
}
else {
  if (scalar(@ARGV) != 2) {
    arg_error("Expecting 2 positional arguments");
  }

  if ($micro) {
    if ($branch eq $default_branch) {
      arg_error("For micro releases, you must define the branch you want to use with --branch");
    }
    $skip_devguide = 1;
    $skip_doxygen = 1;
  }

  $download_url = remove_end_slash($download_url);

  # Process WORKSPACE Argument
  $workspace = $ARGV[0] || "";
  if ($workspace) {
    $workspace = normalizePath($workspace);
  }

  # Process VERSION Argument
  $parsed_version = parse_version($ARGV[1] || "");
  if ($parsed_version) {
    $version = $parsed_version->{string};
    print("Version to release is $version\n");
    $base_name = "${base_name_prefix}$parsed_version->{tag_string}";
    if (!$micro) {
      $release_branch =
        "${release_branch_prefix}${git_name_prefix}$parsed_version->{series_string}";
      if ($parsed_version->{micro} != 0) {
        exit(0) if (!yes_no(
          "Version looks like a micro release, but --micro wasn't passed! Continue?"));
      }
    }
    elsif ($parsed_version->{micro} == 0) {
      exit(0) if (!yes_no(
        "Version looks like a major or minor release, but --micro was passed! Continue?"));
    }
    if (!$next_version) {
      my $next_minor = int($parsed_version->{minor}) + ($micro ? 0 : 1);
      my $next_micro = $micro ? (int($parsed_version->{micro}) + 1) : 0;
      $next_version = sprintf("%s.%d.%d", $parsed_version->{major}, $next_minor, $next_micro);
    }
    $next_version .= "-${metadata}";
    $parsed_next_version = parse_version($next_version);
    if (!$parsed_next_version) {
      arg_error("Invalid next version: $next_version");
    }
    $next_version = $parsed_next_version->{string};
    print("Next after this version is going to be $next_version\n");
  }
  else {
    arg_error("Invalid version: $version");
  }

  if (!$skip_ftp) {
    die("FTP_USERNAME, FTP_PASSWD, FTP_HOST need to be defined")
      if (!(defined($ENV{FTP_USERNAME}) && defined($ENV{FTP_PASSWD}) && defined($ENV{FTP_HOST})));
  }
}

my $release_timestamp = POSIX::strftime($release_timestamp_fmt, gmtime);
$release_timestamp =~ s/  / /g; # Single digit days of the month result in an extra space

my %global_settings = (
    list => $print_list,
    list_all => $print_list_all,
    remedy => $remedy,
    force => $force,
    micro => $micro,
    remote => $remote,
    branch => $branch,
    release_branch => $release_branch,
    github_user => $github_user,
    version => $version,
    parsed_version => $parsed_version,
    next_version => $next_version,
    parsed_next_version => $parsed_next_version,
    base_name => $base_name,
    git_tag => "${git_name_prefix}$parsed_version->{tag_string}",
    clone_dir => join("/", $workspace, ${base_name}),
    tar_src => "${base_name}.tar",
    tgz_src => "${base_name}.tar.gz",
    zip_src => "${base_name}.zip",
    md5_src => "${base_name}.md5",
    sha256_src => "${base_name}.sha256",
    tar_dox => "${base_name}-doxygen.tar",
    tgz_dox => "${base_name}-doxygen.tar.gz",
    zip_dox => "${base_name}-doxygen.zip",
    devguide_ver => "${base_name_prefix}$parsed_version->{series_string}.pdf",
    devguide_lat => "${base_name_prefix}latest.pdf",
    timestamp => $release_timestamp,
    git_url => "git\@github.com:${github_user}/${repo_name}.git",
    github_repo => $repo_name,
    github_token => $ENV{GITHUB_TOKEN},
    ftp_user => $ENV{FTP_USERNAME},
    ftp_password => $ENV{FTP_PASSWD},
    ftp_host => $ENV{FTP_HOST},
    changelog => "docs/history/ChangeLog-$version",
    modified => {
        "NEWS.md" => 1,
        "PROBLEM-REPORT-FORM" => 1,
        "VERSION.txt" => 1,
        "dds/Version.h" => 1,
        "docs/history/ChangeLog-$version" => 1,
    },
    skip_ftp => $skip_ftp,
    skip_devguide => $skip_devguide,
    skip_doxygen => $skip_doxygen,
    skip_website => $skip_website,
    workspace => $workspace,
    workspace_info_file_path => "$workspace/$workspace_info_filename",
    download_url => $download_url,
    ace_root => "$workspace/$ace_root",
    ftp_active => $ftp_active,
    ftp_port => $ftp_port,
    mock => $mock,
    dummy_doxygen => $mock && !$mock_with_doxygen,
    ignore_github_token => $ignore_github_token,
);

if (!$ignore_args) {
  $global_settings{pithub} = new_pithub(\%global_settings);

  check_workspace(\%global_settings);

  if ($mock) {
    if ($github_user eq $default_github_user) {
      die("--github-user can't be left to default when using --mock!");
    }

    if (!$skip_ftp && $download_url eq $default_download_url) {
      die("--download-url can't be left to default when using --mock!");
    }
  }

  my $current_branch = get_current_branch();
  if ($current_branch ne $branch){
    exit(0) if (!yes_no("git is on the $current_branch branch, but the script " .
      "is going to assume it's on the $branch branch, continue?"));
  }
}

my @release_steps = (
  {
    name    => 'Update VERSION.txt File',
    verify  => sub{verify_update_version_file(@_)},
    message => sub{message_update_version_file(@_)},
    remedy  => sub{remedy_update_version_file(@_)},
    can_force => 1,
  },
  {
    name    => 'Update Version.h File',
    verify  => sub{verify_update_version_h_file(@_)},
    message => sub{message_update_version_h_file(@_)},
    remedy  => sub{remedy_update_version_h_file(@_)},
    can_force => 1,
  },
  {
    name    => 'Update PROBLEM-REPORT-FORM File',
    verify  => sub{verify_update_prf_file(@_)},
    message => sub{message_update_prf_file(@_)},
    remedy  => sub{remedy_update_prf_file(@_)},
    can_force => 1,
  },
  {
    name    => 'Verify Remote',
    verify  => sub{verify_git_remote(@_)},
    message => sub{message_git_remote(@_)},
  },
  {
    name    => 'Create ChangeLog File',
    verify  => sub{verify_changelog(@_)},
    message => sub{message_changelog(@_)},
    remedy  => sub{remedy_changelog(@_)},
    can_force => 1,
  },
  {
    name    => 'Update AUTHORS File',
    verify  => sub{verify_authors(@_)},
    message => sub{message_authors(@_)},
    remedy  => sub{remedy_authors(@_)},
    can_force => 1,
  },
  {
    name    => 'Add NEWS File Section',
    verify  => sub{verify_news_file_section(@_)},
    message => sub{message_news_file_section(@_)},
    remedy  => sub{remedy_news_file_section(@_)},
    can_force => 1,
  },
  {
    name    => 'Update NEWS File Section',
    verify  => sub{verify_update_news_file(@_)},
    message => sub{message_update_news_file(@_)},
    can_force => 1,
  },
  {
    name    => 'Update NEWS File Section Timestamp',
    verify  => sub{verify_news_timestamp(@_)},
    message => sub{message_news_timestamp(@_)},
    remedy  => sub{remedy_news_timestamp(@_)},
    can_force => 1,
  },
  {
    name    => 'Commit Release Changes',
    verify  => sub{
      my $settings = shift();
      my $step_options = shift();
      return verify_git_status_clean($settings, {%{$step_options}, strict => 1});
    },
    message => sub{message_commit_git_changes(@_)},
    remedy  => sub{remedy_git_status_clean(@_)}
  },
  {
    name    => 'Create Tag',
    verify  => sub{verify_git_tag(@_)},
    message => sub{message_git_tag(@_)},
    remedy  => sub{remedy_git_tag(@_)}
  },
  {
    name    => 'Push Release Changes',
    prereqs => ['Verify Remote'],
    verify  => sub{verify_git_changes_pushed(@_)},
    message => sub{message_git_changes_pushed(@_)},
    remedy  => sub{remedy_git_changes_pushed(@_)}
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
    prereqs => ['Create Release Branch', 'Verify Remote'],
    verify  => sub{verify_push_release_branch(@_)},
    message => sub{message_push_release_branch(@_)},
    remedy  => sub{remedy_push_release_branch(@_)},
    skip    => !$global_settings{release_branch},
  },
  {
    name    => 'Clone Tag',
    prereqs => ['Verify Remote'],
    verify  => sub{verify_clone_tag(@_)},
    message => sub{message_clone_tag(@_)},
    remedy  => sub{remedy_clone_tag(@_)}
  },
  {
    name    => 'Move ChangeLog File',
    verify  => sub{verify_move_changelog(@_)},
    message => sub{message_move_changelog(@_)},
    remedy  => sub{remedy_move_changelog(@_)}
  },
  {
    name    => 'Create Unix Release Archive',
    verify  => sub{verify_tgz_source(@_)},
    message => sub{message_tgz_source(@_)},
    remedy  => sub{remedy_tgz_source(@_)}
  },
  {
    name    => 'Create Windows Release Archive',
    verify  => sub{verify_zip_source(@_)},
    message => sub{message_zip_source(@_)},
    remedy  => sub{remedy_zip_source(@_)}
  },
  {
    name    => 'Create MD5 Checksum File',
    prereqs => [
      'Create Unix Release Archive',
      'Create Windows Release Archive',
    ],
    verify  => sub{verify_md5_checksum(@_)},
    message => sub{message_md5_checksum(@_)},
    remedy  => sub{remedy_md5_checksum(@_)},
  },
  {
    name => 'Create sha256 Checksum File',
    prereqs => [
      'Create Unix Release Archive',
      'Create Windows Release Archive',
    ],
    verify  => sub{verify_sha256_checksum(@_)},
    message => sub{message_sha256_checksum(@_)},
    remedy  => sub{remedy_sha256_checksum(@_)},
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
    name    => 'Generate Doxygen',
    prereqs => ['Extract OCI ACE/TAO'],
    verify  => sub{verify_gen_doxygen(@_)},
    message => sub{message_gen_doxygen(@_)},
    remedy  => sub{remedy_gen_doxygen(@_)},
    skip    => $global_settings{skip_doxygen},
  },
  {
    name    => 'Create Unix Doxygen Archive',
    prereqs => ['Generate Doxygen'],
    verify  => sub{verify_tgz_doxygen(@_)},
    message => sub{message_tgz_doxygen(@_)},
    remedy  => sub{remedy_tgz_doxygen(@_)},
    skip    => $global_settings{skip_doxygen},
  },
  {
    name    => 'Create Windows Doxygen Archive',
    prereqs => ['Generate Doxygen'],
    verify  => sub{verify_zip_doxygen(@_)},
    message => sub{message_zip_doxygen(@_)},
    remedy  => sub{remedy_zip_doxygen(@_)},
    skip    => $global_settings{skip_doxygen},
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
    remedy  => sub{remedy_github_upload(@_)},
  },
  {
    name    => 'Release Website',
    verify  => sub{verify_website_release(@_)},
    message => sub{message_website_release(@_)},
    remedy  => sub{remedy_website_release(@_)},
    skip    => $global_settings{skip_website},
  },
  {
    name => 'Record that the Release Occurred',
    verify => sub{verify_release_occurred_flag(@_)},
    message => sub{return "Release ouccured flag needs to be set"},
    remedy => sub{remedy_release_occurred_flag(@_)},
  },
  {
    name    => 'Update NEWS for Post-Release',
    verify  => sub{verify_news_template_file_section(@_)},
    message => sub{message_news_template_file_section(@_)},
    remedy  => sub{remedy_news_template_file_section(@_)},
    post_release => 1,
  },
  {
    name    => 'Update VERSION.txt for Post-Release',
    verify  => sub{
      my $settings = shift();
      my $step_options = shift();
      return verify_update_version_file($settings, {%{$step_options}, post_release => 1});
    },
    message => sub{message_update_version_file(@_)},
    remedy  => sub{remedy_update_version_file(@_, 1)},
    can_force => 1,
    post_release => 1,
  },
  {
    name    => 'Update Version.h for Post-Release',
    verify  => sub{
      my $settings = shift();
      my $step_options = shift();
      return verify_update_version_h_file($settings, {%{$step_options}, post_release => 1});
    },
    message => sub{message_update_version_h_file(@_)},
    remedy  => sub{remedy_update_version_h_file(@_, 1)},
    can_force => 1,
    post_release => 1,
  },
  {
    name    => 'Update PROBLEM-REPORT-FORM for Post-Release',
    verify  => sub{
      my $settings = shift();
      my $step_options = shift();
      return verify_update_prf_file($settings, {%{$step_options}, post_release => 1});
    },
    message => sub{message_update_prf_file(@_, 1)},
    remedy  => sub{remedy_update_prf_file(@_, 1)},
    can_force => 1,
    post_release => 1,
  },
  {
    name    => 'Commit Post-Release Changes',
    verify  => sub{
      my $settings = shift();
      my $step_options = shift();
      verify_git_status_clean($settings, {%{$step_options}, strict => 1});
    },
    message => sub{message_commit_git_changes(@_)},
    remedy  => sub{remedy_git_status_clean(@_, 1)},
    post_release => 1,
  },
  {
    name    => 'Push Post-Release Changes',
    prereqs => ['Verify Remote'],
    verify  => sub{verify_git_changes_pushed(@_)},
    message => sub{message_git_changes_pushed(@_)},
    remedy  => sub{remedy_git_changes_pushed(@_, 0)},
    post_release => 1,
  },
  {
    name    => 'Email DDS-Release-Announce list',
    verify  => sub{verify_email_list(@_)},
    message => sub{message_email_dds_release_announce(@_)},
    remedy  => sub{remedy_email_dds_release_announce(@_)},
    post_release => 1,
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
  if (not exists $step->{post_release}) {
    $step->{post_release} = 0;
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
  my $settings = shift();
  my $release_steps = shift();
  my $step_number = shift();
  my $run_count_ref = shift();
  my $skip_count_ref = shift();
  my $fail_count_ref = shift();

  my $step = $release_steps->[$step_number-1];
  my $title = $step->{name};

  if (!$settings->{list_all} && ($step->{skip} || $step->{verified} ||
      ($settings->{release_occurred} && !$step->{post_release}))) {
    ++${$skip_count_ref};
    return;
  }

  print "$step_number: $title\n";
  return if $settings->{list};

  ++${$run_count_ref};

  # Check Prerequisite Steps
  if ($step->{prereqs}) {
    foreach my $prereq_name (@{$step->{prereqs}}) {
      run_step($settings, $release_steps, get_step_by_name($prereq_name));
    }
  }

  # Run the verification
  if (!$step->{verify}($settings, {remedy_verify => 0})) {
    # Failed
    print "$divider\n";
    print "  " . $step->{message}($settings) . "\n";

    my $remedied = 0;
    my $forced = 0;
    # If a remedy is available and --remedy specified
    if ($step->{remedy} && $settings->{remedy}) {
      # Try remediation
      if (!$step->{remedy}($settings)) {
        print "  !!!! Remediation did not complete\n";
      # Reverify
      }
      elsif (!$step->{verify}($settings, {remedy_verify => 1})) {
        print "  !!!! Remediation did not pass verification\n";
      }
      else {
        print "  Done!\n";
        $remedied = 1;
      }
    }

    if (!$remedied) {
      ++${$fail_count_ref};

      # If not remedied, try forcing
      if ($step->{can_force} && $settings->{force}) {
        $forced = 1;
        print "Forcing Past This Step!\n";
      }
    }

    if (!($remedied || $forced)) {
      if ($step->{remedy}) {
        if (!$settings->{remedy}) {
          print "  Use --remedy to attempt a fix\n";
        }
      }
      else {
        print "  Step can NOT be automatically resolved using --remedy\n";
      }
      if ($step->{can_force}) {
        if (!$settings->{force}) {
          print "  Use --force to force past this step\n";
        }
      }
      else {
        print "  Step can NOT be forced using --force\n";
      }
    }

    die unless ($remedied || $forced);
    print "$divider\n";
  }

  print "  Step $step_number \"$title\" is OK!\n";

  $step->{verified} = 1;
}

if ($ignore_args || ($workspace && $parsed_version)) {
  my @steps_to_do;
  my $no_steps = scalar(@release_steps);
  if ($step_expr) {
    @steps_to_do = parse_step_expr($no_steps, $step_expr);
  }
  else {
    @steps_to_do = 1..$no_steps;
  }

  my $run_count = 0;
  my $skip_count = 0;
  my $fail_count = 0;
  foreach my $step_number (@steps_to_do) {
    run_step(\%global_settings, \@release_steps, $step_number,
      \$run_count, \$skip_count, \$fail_count);
  }

  print("Ran: $run_count\n") if ($run_count);
  print("Skipped: $skip_count\n") if ($skip_count);
  print("Failed: $fail_count\n") if ($fail_count);
}
else {
  $status = 1;
  print STDERR "ERROR: Invalid Arguments, see --help for Valid Usage\n";
}

exit $status;
