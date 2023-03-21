#!/usr/bin/env perl

use strict;
use warnings;

use POSIX;
use Cwd;
use File::Basename;
use File::stat;
use File::Temp qw/tempdir/;
use File::Path qw/make_path remove_tree/;
use File::Copy qw/copy/;
use File::Find qw/find/;
use Getopt::Long;
use JSON::PP;
use Storable qw/dclone/;

use LWP::UserAgent;
use Net::SFTP::Foreign;
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
my $default_github_user = "OpenDDS";
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
my $default_sftp_base_dir = "";
my $sftp_downloads_path = "downloads/OpenDDS";
my $sftp_previous_releases_path = 'previous-releases/';
my $rtd_url_base = 'https://readthedocs.org/api/v3/';
my $rtd_project_url = "${rtd_url_base}projects/opendds/";
my $json_mime = 'application/json';

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
    "gitrelease.pl --list-all\n" .
    "gitrelease.pl --update-authors\n";
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
    "  --list-all             Same as --list, but show every step regardless of other\n" .
    "                         options. This doesn't require the WORKSPACE or VERSION\n" .
    "                         arguments\n" .
    "  --steps STEPS          Optional, Steps to perform, default is all\n" .
    "                         See \"Step Expressions\" Below for what it accepts.\n" .
    "  --remedy               Remediate problems where possible\n" .
    "  --force                Keep going if possible\n" .
    "  --remote=NAME          Valid git remote for OpenDDS (default: ${default_remote})\n" .
    "  --branch=NAME          Valid git branch for cloning (default: ${default_branch})\n" .
    "  --github-user=NAME     User or organization name for github updates\n" .
    "                         (default: ${default_github_user})\n" .
    "  --download-url=URL     URL to verify SFTP uploads\n" .
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
    "  --skip-sftp            Skip the SFTP upload\n" .
    "  --sftp-base-dir        Change to this directory before file operations.\n".
    "                         (default: \"$default_sftp_base_dir\")\n" .
    "  --skip-github [1|0]    Skip anything having to do with GitHub\n" .
    "                         GitHub is used to determine if the release is the\n" .
    "                         highest version and if --skip-github 1 is passed it will\n" .
    "                         be assumed it's always the highest version. Passing 0\n" .
    "                         forces it to consider this to be a update to an earlier\n" .
    "                         micro series.\n" .
    "  --upload-shapes-demo   Check GitHub for a finished shapes demo build workflow\n" .
    "                         run. Then download, repackage, and upload the results\n" .
    "                         to the download server. This doesn't run any steps,\n" .
    "                         but still requires the WORKSPACE and VERSION arguments\n" .
    "                         and accepts and uses other relevant options.\n" .
    "  --update-authors       Just update the AUTHORS files like in a release.\n" .
    "                         This doesn't run any release steps and doesn't require\n" .
    "                         the WORKSPACE or VERSION arguments.\n" .
    "\n" .
    "Environment Variables:\n" .
    "  GITHUB_TOKEN           GitHub token with repo access to publish release on\n" .
    "                         GitHub.\n" .
    # When this is ready also remove fixed skip on RTD step
    # "  READ_THE_DOCS_TOKEN    Access token for Read the Docs Admin\n" .
    "  SFTP_USERNAME          SFTP Username\n" .
    "  SFTP_HOST              SFTP Server Address\n" .
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
  return join("", @lines);
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

sub die_with_stack_trace {
  my $i = 1;
  print STDERR ("ERROR: ", @_, " STACK TRACE:\n");
  while (my @call_details = (caller($i++)) ){
      print STDERR "ERROR: STACK TRACE[", $i - 2, "] " .
        "$call_details[1]:$call_details[2] in function $call_details[3]\n";
  }
  die();
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

  return undef if ($settings->{skip_github});

  if (!defined($settings->{github_token})) {
    die("GITHUB_TOKEN must be defined");
  }

  return Pithub->new(
    user => $settings->{github_user},
    repo => $settings->{github_repo},
    token => $settings->{github_token},
  );
}

sub check_pithub_response {
  my $response = shift();
  my $extra_msg = shift();

  unless ($response->is_success) {
    die_with_stack_trace(
      "HTTP issue while using PitHub: \"", $response->status_line(), "\"$extra_msg\n");
  }
}

sub check_pithub_result {
  my $result = shift();
  my $extra_msg = shift() // '';

  check_pithub_response($result->response, $extra_msg);
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
    force_not_highest_version => $settings->{force_not_highest_version},
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
    $invalid |= compare_workspace_info($settings, $info, 'force_not_highest_version');
    if ($invalid) {
      die("Inconsistent with existing workspace, see above for details");
    }

    $settings->{is_highest_version} = $info->{is_highest_version};
    if ($info->{force_not_highest_version} && $info->{is_highest_version}) {
      die("force_not_highest_version and is_highest_version can't both be true");
    }
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
    if ($settings->{force_not_highest_version}) {
      $settings->{is_highest_version} = 0;
    }
    $settings->{release_occurred} = 0;
    write_workspace_info($settings);
  }
}

sub download {
  my $url = shift();
  my %args = @_;

  my $agent = LWP::UserAgent->new();
  if ($args{curl_user_agent}) {
    $agent->agent('curl/7.71.0');
  }

  my @header = ();
  my $req_content = undef;

  if (exists($args{token})) {
    push(@header, Authorization => "Token $args{token}");
  }

  if (exists($args{req_json_ref})) {
    $args{req_content_ref} = [$json_mime, JSON::PP::encode_json($args{req_json_ref})];
  }
  if (exists($args{req_content_ref})) {
    push(@header, 'Content-Type' => $args{req_content_ref}->[0]);
    $req_content = $args{req_content_ref}->[1];
  }

  my $method = $args{method} // 'GET';
  my $request = HTTP::Request->new($method, $url, \@header, $req_content);

  my $to = "";
  my $content_file = undef;
  if (exists($args{save_path})) {
    $content_file = $args{save_path};
    $to = " to \"$args{save_path}\"";
  }

  print("Downloading ($method) $url$to...\n");
  if ($args{debug}) {
    print($request->as_string());
  }
  my $response = $agent->request($request, $content_file);
  if ($args{debug}) {
    print($response->as_string());
  }
  if (exists($args{response_ref})) {
    ${$args{response_ref}} = $response;
  }
  if (!$response->is_success()) {
    print STDERR "ERROR: ", $response->status_line(), "\n";
    print STDERR "CONTENT TYPE: ", $response->content_type(), "\n";
    print STDERR "CONTENT ", "=" x 72, "\n", $response->content(),
      "\nEND CONTENT ", "=" x 68, "\n";

    return 0;
  }

  if (exists($args{content_ref})) {
    ${$args{content_ref}} = $response->decoded_content();
  }

  if (exists($args{res_json_ref})) {
    if ($response->content_type() ne $json_mime) {
      print STDERR "ERROR: Expected $json_mime response, not ", $response->content_type(), "\n";
      print STDERR "CONTENT ", "=" x 72, "\n", $response->content(),
        "\nEND CONTENT ", "=" x 68, "\n";
      return 0;
    }
    ${$args{res_json_ref}} = JSON::PP::decode_json($response->content());
  }

  return 1;
}

sub get_current_branch {
  my %args = @_;
  my $opts = '';
  if (!$args{get_hash}) {
    $opts .= ' --abbrev-ref ';
  }
  open(my $fh, "git rev-parse${opts} HEAD|") or die "git: $!";
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

sub create_archive {
  my $src_dir = shift();
  my $arc = shift();
  my %args = @_;
  my $exclude = $args{exclude};

  my $arc_name = basename($arc);
  my $src_dir_parent = dirname($src_dir);
  my $src_dir_name = basename($src_dir);

  print("  Creating archive $arc_name from $src_dir...\n");

  my $chdir = ChangeDir->new($src_dir_parent);

  my $cmd;
  if ($arc_name =~ /\.tar\.gz$/) {
    $cmd = ['tar', '--create', '--use-compress-program', 'gzip -9'];
    if (defined($exclude)) {
      for my $e (@{$exclude}) {
        my $copy = "$src_dir_name/$e";
        push(@{$cmd}, '--exclude', $copy);
      }
    }
    push(@{$cmd}, '--file', $arc, $src_dir_name);
  }
  elsif ($arc_name =~ /\.zip$/) {
    $cmd = ['zip', '--quiet', '-9', '--recurse-paths', $arc, $src_dir_name];
    if (defined($exclude)) {
      push(@{$cmd}, '--exclude');
      for my $e (@{$exclude}) {
        my $copy = "./$src_dir_name/$e";
        if (-e $copy && -d $copy) {
          $copy .= '/';
        }
        push(@{$cmd}, $copy);
      }
    }
  }
  else {
    die("Can't guess archive type from name: $arc_name");
  }

  return run_command($cmd);
}

sub verify_archive {
  my $src_dir = shift();
  my $arc = shift();
  my %args = @_;
  my $exclude = $args{exclude};
  my $remedy_verify = $args{remedy_verify};
  my $zip_file_contents = $args{zip_file_contents};
  my $files_differ_ref = $args{files_differ};

  my $arc_name = basename($arc);
  print("  Verifying that archive $arc_name matches $src_dir...\n");

  my $missing_src_dir = !-d $src_dir;
  print STDERR ("ERROR: verify_archive: $src_dir for $arc doesn't exist\n")
    if ($missing_src_dir && $remedy_verify);
  my $missing_arc = !-f $arc;
  print STDERR ("ERROR: verify_archive: $arc doesn't exist\n")
    if ($missing_arc && $remedy_verify);
  # Rest of the function should probably not use remedy_verify to better inform
  # the user why an archive is being recreated.
  return 0 if ($missing_src_dir || $missing_arc);

  my $src_dir_parent = dirname($src_dir);
  my $src_dir_name = basename($src_dir);
  my $chdir = ChangeDir->new($src_dir_parent);

  my $same_existing_files = 1;
  my @archive_array;

  if ($arc_name =~ /\.tar\.gz$/) {
    # This compares the files in the archive to the source directory
    $same_existing_files = 0
      if (!run_command(['tar', '--compare', '--file', $arc, $src_dir_name]));

    my $listing;
    run_command(['tar', '--list', '--file', $arc, $src_dir_name],
      capture => {stdout => \$listing}, autodie => 1);
    @archive_array = split("\n", $listing);
  }
  elsif ($arc_name =~ /\.zip$/) {
    # This is just a simple integrity check
    run_command(['unzip', '-t', "$arc"], capture => {stdout => undef}) or return 0;

    # This compares the files in the archive to the source directory
    my $listing;
    run_command(['zipinfo', '-1', $arc], capture => {stdout => \$listing}, autodie => 1);
    @archive_array = split("\n", $listing);
    for my $path (@archive_array) {
      if ($path =~ /\/$/) {
        if (!-d $path) {
          print STDERR ("verify_archive: $arc has deleted dir $path\n");
          $same_existing_files = 0;
        }
      }
      elsif (-f $path) {
        if ($zip_file_contents && !run_command("unzip -p $arc $path | diff --brief $path -")) {
          $same_existing_files = 0
        }
      }
      else {
        print STDERR ("verify_archive: $arc has deleted file $path\n");
        $same_existing_files = 0;
      }
    }
  }
  else {
    die("Can't guess archive type from name: $arc_name");
  }

  my %excluded;
  if (defined($exclude)) {
    for my $e (@{$exclude}) {
      $excluded{"$src_dir_name/$e"} = undef;
    }
  }

  # The previous checks checked that the existing files from the archive
  # matched, this checks for files that should or shouldn't be in the archive.
  my $missing_success = 1;
  my %archive_hash;
  for my $path (@archive_array) {
    $path =~ s/\/$//;
    $archive_hash{$path} = undef;
  }
  find(sub {
    my $path = $File::Find::name;
    my $is_excluded = exists($excluded{$path});
    if (exists($archive_hash{$path})) {
      if ($is_excluded) {
        print STDERR ("$path should've been excluded from $arc_name\n");
        $missing_success = 0;
      }
    }
    elsif (!$is_excluded) {
      print STDERR ("$path could not be found in $arc_name\n");
      $missing_success = 0;
    }
  }, $src_dir_name);

  my $files_same = $same_existing_files && $missing_success;
  if ($files_differ_ref) {
    ${$files_differ_ref} = !$files_same;
  }
  return $files_same;
}

sub extract_archive {
  my $arc = shift();
  my $dst_dir = shift();

  my $arc_name = basename($arc);
  my $chdir = ChangeDir->new($dst_dir);

  my $cmd;
  if ($arc_name =~ /\.zip$/) {
    $cmd = ['unzip', $arc];
  }
  else {
    die("Can't guess archive type from name: $arc_name");
  }

  return run_command($cmd);
}

sub sftp_missing {
  my $settings = shift();
  my $sub_dir = shift();

  my $url = "$settings->{download_url}/";
  if ($url !~ /^https?:\/\//) {
    $url = "http://$url";
  }

  my $content;
  if (!download($url, content_ref => \$content)) {
    die("Couldn't get SFTP contents from $url");
  }

  my @missing;
  for my $file (@_) {
    if ($content !~ /$file/) {
      push(@missing, $file);
    }
  }

  return @missing;
}

sub new_sftp {
  my $settings = shift();

  my $sftp = Net::SFTP::Foreign->new(
    host => $settings->{sftp_host},
    user => $settings->{sftp_user},
    autodie => 1,
  );

  $sftp->setcwd($settings->{sftp_base_dir});
  $sftp->setcwd($sftp_downloads_path);

  return $sftp;
}

sub get_actions_url {
  my $settings = shift();

  my $ph = $settings->{pithub};
  return "/repos/$ph->{user}/$ph->{repo}/actions";
}

sub is_shapes_workflow_path {
  my $path = shift();
  return $path =~ /ishapes/;
}

sub trigger_shapes_run {
  my $settings = shift();

  # https://docs.github.com/en/rest/actions/workflows#create-a-workflow-dispatch-event
  my $response = $settings->{pithub}->request(
    method => 'POST',
    path => get_actions_url($settings) . "/workflows/ishapes.yml/dispatches",
    data => {
      ref => $settings->{git_tag},
    },
  );
  check_pithub_response($response);
}

sub get_last_shapes_run {
  my $settings = shift();

  my $response = $settings->{pithub}->request(
    method => 'GET',
    path => get_actions_url($settings) . '/runs',
    params => {
      branch => $settings->{git_tag},
    },
  );
  check_pithub_response($response);
  for my $run (@{$response->content->{workflow_runs}}) {
    if (is_shapes_workflow_path($run->{path})) {
      print("Last workflow run for $settings->{git_tag} was $run->{html_url}\n");
      return $run;
    }
  }

  return undef;
}

sub upload_shapes_demo {
  my $settings = shift();

  my $ws_subdir = "$settings->{workspace}/shapes-demo";
  if (-d $ws_subdir) {
    remove_tree($ws_subdir);
  }
  mkdir($ws_subdir) or die ("mkdir failed: $!");

  my $ph = $settings->{pithub};
  my $actions_url = get_actions_url($settings);

  # Get the last successful shapes run
  my $run = get_last_shapes_run($settings);
  if (!defined($run)) {
    die("No shapes demo run found!");
  }
  if ($run->{status} ne 'completed') {
    print("It's listed as not finished, run this again when it finishes successfully.\n");
    exit(0);
  }
  if ($run->{conclusion} ne 'success') {
    die("Conclusion is listed as: $run->{conclusion}\n",
      "Try rerunning the specfic job that failed?\n");
  }

  # Get the artifacts from that run
  my $response = $ph->request(
    method => 'GET',
    path => "$actions_url/runs/$run->{id}/artifacts",
  );
  check_pithub_response($response);
  my @artifacts;
  for my $artifact (@{$response->content->{'artifacts'}}) {
    print("FOUND ID: $artifact->{id} NAME: $artifact->{name}\n");
    my $filename = "$artifact->{name}.zip";
    push(@artifacts, {
      id => $artifact->{id},
      name => $artifact->{name},
      filename => $filename,
      path => "$ws_subdir/$filename",
    });
  }
  for my $artifact (@artifacts) {
    print("Getting $artifact->{filename}...\n");
    $response = $ph->request(
      method => 'GET',
      path => "$actions_url/artifacts/$artifact->{id}/zip",
    );
    check_pithub_response($response);
    open(my $fd, '>', $artifact->{path}) or die ("Could not open $artifact->{path}: $!");
    binmode($fd);
    print $fd $response->raw_content();
    close($fd);
  }

  # Prepare and archive demo files
  my $base_name = "ShapesDemo-${base_name_prefix}$settings->{version}-";
  my @to_upload = ();
  for my $artifact (@artifacts) {
    my $os_name;
    if ($artifact->{name} =~ /^(.*)_artifact$/) {
      $os_name = $1;
    }
    else {
      die("Unexpected artifact name: $artifact->{name}");
    }
    my $dir_name = $base_name . $os_name;
    my $dir_path = "$ws_subdir/$dir_name";
    mkdir($dir_path) or die ("mkdir failed: $!");

    print("Unzipping $artifact->{filename} to $dir_name...\n");
    extract_archive($artifact->{path}, $dir_path) or die("Extract artifact failed");

    my $ishapes_src = 'examples/DCPS/ishapes';
    my @files = (
      "$ishapes_src/SHAPESDEMO_README",
    );
    for my $file (@files) {
      my $filename = basename($file);
      copy($file, "$dir_path/$filename") or die("copy $filename failed: $!");
    }

    print("Archiving $dir_name...\n");
    my $arc_ext;
    if ($os_name =~ /windows/i) {
      $arc_ext = '.zip';
    }
    elsif ($os_name =~ /linux/i) {
      $arc_ext = '.tar.gz';
      # Make sure the executable has the exec bit
      chmod(0755, "$dir_path/ishapes") or die("Failed to chmod ishapes: $!");
    }
    else {
      die("Can't derive archive type from artifact name: $os_name (new OS?)");
    }
    my $arc = $dir_path . $arc_ext;
    push(@to_upload, $arc);
    create_archive($dir_path, $arc) or die("Failed to create demo archive $arc");
  }

  my $sftp = new_sftp($settings);
  my $dest_dir = 'ShapesDemo';
  $sftp->mkpath($dest_dir);
  $sftp->setcwd($dest_dir);

  # Delete old files
  foreach my $file (map {$_->{filename}} @{$sftp->ls()}) {
    if ($file =~ /^ShapesDemo-/) {
      print("deleting $file\n");
      $sftp->remove($file);
    }
  }

  # Upload new ones
  foreach my $upload (@to_upload) {
    my $name = basename($upload);
    print("putting $name\n");
    $sftp->put($upload, $name);
  }
}

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
  check_pithub_result($release_list);

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
# Start of Release Steps

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
  my $unclean_ref = $step_options->{unclean};
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

  if ($unclean_ref) {
    ${$unclean_ref} = $unclean;
  }
  else {
    $settings->{unclean} = $unclean;
  }
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
    system("git add $settings->{changelog}") == 0
      or die "Could not execute: git add $settings->{changelog}";
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
  return \@global_authors if (@global_authors) ;

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

      # Prefer Actual Emails to GitHub Accounts
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

  # Replace GitHub Account emails with url to their profiles
  foreach my $author (@authors) {
    if ($author->{email} =~ /$github_email_re/) {
      $author->{email} = "https://github.com/$1";
    }
  }

  @global_authors = sort { name_key($a->{name}) cmp name_key($b->{name}) } @authors;
  return \@global_authors;
}

sub author_lines {
  my @lines;
  foreach my $author (@{get_authors()}) {
    push(@lines, "$author->{name} <$author->{email}>");
  }
  return \@lines;
}

sub verify_authors {
  my $settings = shift();

  my $tmp_authors_path = "$settings->{workspace}/temp_authors";
  open(my $file, '>', $tmp_authors_path) or die ("Could not open $tmp_authors_path: $!");
  foreach my $line (@{author_lines()}) {
    print $file "$line\n";
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
  foreach my $line (@{author_lines()}) {
    print $authors "$line\n";
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
  if (!$settings->{skip_github} && !$found && verify_push_release_branch($settings)) {
    print("$settings->{release_branch} not here, but already pushed\n");
    return 1;
  }
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

  my $find = get_current_branch(get_hash => 1) . "\trefs/heads/$settings->{release_branch}";
  my $found;
  open(GIT, "git ls-remote --heads $settings->{remote} |") or die "Opening $!";
  while (<GIT>) {
    chomp;
    if ($_ eq $find) {
      $found = 1;
      last;
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

sub release_archive_values {
  my $which = shift();
  my $settings = shift();

  my $worktree = $settings->{"${which}_worktree"};
  my $arc_name = $settings->{"${which}_src"};
  return (
    $worktree,
    $arc_name,
    "$settings->{workspace}/$arc_name",
    "$worktree/ChangeLog",
    (($which eq 'zip') ? 1 : 0),
  );
}

sub release_archive_worktree_verify {
  my $which = shift();
  my $settings = shift();

  my ($worktree, $arc_name, $arc_path, $cl, $crlf) = release_archive_values($which, $settings);

  my $correct = 0;
  if (-d $worktree) {
    my $chdir = ChangeDir->new($worktree);
    # Using git describe for compatibility with older versions of git
    open(GIT_DESCRIBE, "git describe --tags |") or die "git describe --tags $!";
    while (<GIT_DESCRIBE>) {
      if (/^$settings->{git_tag}/) {
        $correct = 1;
        next;
      }
    }
    close GIT_DESCRIBE;

    $correct = 0 if (!-f $cl);

    my $check_file = "$worktree/VERSION.txt";
    if (-f $check_file) {
      open(my $fd, $check_file) or die("Couldn't open $check_file: $!");
      my $line = <$fd>;
      my $found_crlf = (($line =~ /\r\n/) ? 1 : 0);
      close($fd);
      if ($found_crlf != $crlf) {
        print STDERR ('ERROR: unexpected ', $found_crlf ? 'DOS' : 'UNIX',
          " line endings in $check_file\n");
        $correct = 0;
      }
    }
    else {
      $correct = 0;
    }

    my $unclean;
    if (!verify_git_status_clean($settings, {strict => 1, unclean => \$unclean})) {
      print STDERR ("ERROR: This release archive work tree $worktree is not clean:\n" .
        "${unclean}Undo these changes so the archives will match the tag as committed\n");
      die("stopped so we don't regenerate the archive");
    }
  }
  return $correct;
}

sub release_archive_worktree_message {
  my $which = shift();
  my $settings = shift();

  my ($worktree, $arc_name, $arc_path, $cl, $crlf) = release_archive_values($which, $settings);

  my $message = "$worktree ";
  if (-d $worktree) {
    $message .= "is either invalid and needs to be recreated";
  }
  else {
    $message .= "needs to be created";
  }
  return $message;
}

sub release_archive_worktree_remedy {
  my $which = shift();
  my $settings = shift();

  my ($worktree, $arc_name, $arc_path, $cl, $crlf) = release_archive_values($which, $settings);

  if (-d $worktree) {
    run_command("git worktree remove --force $worktree", autodie => 1);
  }

  my $crlf_insert = '';
  if ($crlf) {
    $crlf_insert = "-c core.autocrlf=true ";
  }
  run_command("git ${crlf_insert}worktree add $worktree $settings->{git_tag}", autodie => 1);
  if ($crlf) {
    my $chdir = ChangeDir->new($worktree);
    run_command("git config --local extensions.worktreeConfig true", autodie => 1);
    run_command("git config --worktree core.autocrlf true", autodie => 1);
  }
  copy("$worktree/$settings->{changelog}", $cl) or die("copy $cl failed: $!");

  return 1;
}

############################################################################

my $exclude_from_release_arc = [
  '.git',
  'docs/doxygen_ace_tao_generated_links.h',
];

sub release_archive_verify {
  my $which = shift();
  my $settings = shift();
  my $step_options = shift() // {};

  my ($worktree, $arc_name, $arc_path, $cl, $crlf) = release_archive_values($which, $settings);
  my $differ_key = "release_archive_files_differ_prompt_$which";

  my $files_differ = 0;
  my $result = verify_archive($worktree, $arc_path,
    exclude => $exclude_from_release_arc,
    remedy_verify => $step_options->{remedy_verify},
    files_differ => \$files_differ);
  if ($files_differ && !exists($settings->{$differ_key})) {
    $settings->{$differ_key} = 1;
  }
  return $result;
}

sub release_archive_message {
  my $which = shift();
  my $settings = shift();

  my ($worktree, $arc_name, $arc_path, $cl, $crlf) = release_archive_values($which, $settings);

  my $message = "$arc_name ";
  if (-f $arc_path) {
    $message .= "is either invalid or doesn't match $worktree and needs to be recreated";
  }
  else {
    $message .= "needs to be created";
  }
  return $message;
}

sub release_archive_remedy {
  my $which = shift();
  my $settings = shift();

  my ($worktree, $arc_name, $arc_path, $cl, $crlf) = release_archive_values($which, $settings);
  my $differ_key = "release_archive_files_differ_prompt_$which";

  if ($settings->{$differ_key}) {
    exit(0) if (!yes_no("$arc_name will be recreated becuase of source directory changes, Continue?"));
    $settings->{$differ_key} = 0;
  }
  return create_archive($worktree, $arc_path, exclude => $exclude_from_release_arc);
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

  if ($settings->{dummy_doxygen}) {
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
  return "$settings->{doxygen_inner_dir}/index.html";
}

sub verify_gen_doxygen {
  my $settings = shift();
  return -f doxygen_sanity_file($settings);
}

sub message_gen_doxygen {
  my $settings = shift();
  return "Doxygen documentation needs generating in dir $settings->{doxygen_dir}\n";
}

sub remedy_gen_doxygen {
  my $settings = shift();

  if ($settings->{dummy_doxygen}) {
    my $file = doxygen_sanity_file($settings);
    print "Touch $file because of mock release\n";
    touch_file($file);
    return 1;
  }

  my $dox_dir = $settings->{doxygen_dir};
  mkdir($dox_dir) if (!-d $dox_dir);
  $ENV{DDS_ROOT} = $settings->{tgz_worktree};
  $ENV{ACE_ROOT} = $settings->{ace_root};
  run_command(["./tools/scripts/generate_combined_doxygen.pl", $dox_dir, '-is_release'],
    chdir => $ENV{DDS_ROOT}, autodie => 1);

  print "Converting doxygen files to Windows line endings...\n";
  my $converter = new ConvertFiles();
  my ($stat, $error) = $converter->convert($dox_dir);
  if (!$stat) {
    print $error;
    return 0;
  }

  return 1;
}

############################################################################

sub dox_values {
  my $settings = shift();
  return ($settings->{doxygen_inner_dir}, "$settings->{workspace}/$settings->{zip_dox}");
}

sub verify_zip_doxygen {
  my $settings = shift();
  my $step_options = shift() // {};
  return verify_archive(dox_values($settings), remedy_verify => $step_options->{remedy_verify});
}

sub message_zip_doxygen {
  my $settings = shift();

  my ($dox_dir, $dox_zip) = dox_values($settings);
  my $message = "$settings->{zip_dox} ";
  if (-f $dox_zip) {
    $message .= "is either invalid or doesn't match source directory and needs to be recreated";
  }
  else {
    $message .= "needs to be created";
  }
  return $message;
}

sub remedy_zip_doxygen {
  my $settings = shift();
  return create_archive(dox_values($settings));
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

  # Official IANA list: https://www.iana.org/assignments/media-types/media-types.xhtml
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
  }

  return @files;
}

sub get_sftp_release_files {
  my $settings = shift();

  my @files = get_github_release_files($settings);

  if (!$settings->{skip_devguide}) {
    push(@files, $settings->{devguide_lat}) if ($settings->{is_highest_version});
  }
  if (!$settings->{skip_doxygen}) {
    push(@files, $settings->{zip_dox});
  }

  return @files;
}

sub verify_sftp_upload {
  my $settings = shift();

  my $sub_dir = "";
  $sub_dir = $sftp_previous_releases_path if (!$settings->{is_highest_version});
  my @missing = sftp_missing($settings, $sub_dir, get_sftp_release_files($settings));
  for my $file (@missing) {
    print "$file not found\n";
  }
  return scalar(@missing) ? 0 : 1;
}

sub message_sftp_upload {
  return "Release needs to be uploaded to SFTP site";
}

sub remedy_sftp_upload {
  my $settings = shift();

  my $sftp = new_sftp($settings);

  my @current_release_files = map {$_->{filename}} @{$sftp->ls()};
  my @new_release_files = get_sftp_release_files($settings);

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
            my $new_path = $sftp_previous_releases_path . $file;
            print "moving $file to $new_path\n";
            $sftp->rename($file, $new_path);
          }
          else {
            print "deleting $file\n";
            $sftp->remove($file);
          }
          last;
        }
      }
    }
  }
  else {
    print "Not highest version release, cd-ing to $sftp_previous_releases_path\n";
    $sftp->setcwd($sftp_previous_releases_path);
  }

  # upload new release files
  foreach my $file (@new_release_files) {
    print "uploading $file\n";
    my $local_file = join("/", $settings->{workspace}, $file);
    $sftp->put($local_file, $file);
  }

  return 1;
}

############################################################################

sub verify_github_upload {
  my $settings = shift();
  my $verified = 0;

  my $release_list = $settings->{pithub}->repos->releases->list();
  check_pithub_result($release_list);
  while (my $row = $release_list->next) {
    if ($row->{tag_name} eq $settings->{git_tag}){
      #printf "%d\t[%s]\n",$row->{id},$row->{tag_name};
      $verified = 1;
      last;
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
  check_pithub_result($release);

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
    check_pithub_result($asset, $fail_msg);
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

sub rtd_api {
  my $settings = shift();
  my $method = shift();
  my $suburl = shift();
  my %args = @_;

  if (!defined($settings->{read_the_docs_token})) {
    die("READ_THE_DOCS_TOKEN must be defined");
  }

  my $response;
  return 0 unless(download(
    $rtd_project_url . $suburl,
    method => $method,
    curl_user_agent => 1, # Read the Docs gives back a weird error without this
    token => $settings->{read_the_docs_token},
    response_ref => \$response,
    @_,
  ));

  if (exists($args{expected_status}) && $response->code() != $args{expected_status}) {
    print STDERR "ERROR: Expected HTTP Status $args{expected_code}, got ", $response->code(), "\n";
    return 0;
  }

  return 1;
}

sub rtd_api_version {
  my $settings = shift();
  my $method = shift();

  my $name = lc($settings->{git_tag});
  return rtd_api($settings, $method, "versions/$name/", @_);
}

sub verify_rtd_activate {
  my $settings = shift();

  my $version_info;
  die("Failed to get version info") unless(rtd_api_version(
    $settings, 'GET',
    res_json_ref => \$version_info,
  ));
  return $version_info->{active} && !$version_info->{hidden};
}

sub message_rtd_activate {
  my $settings = shift();
  return "Read the docs version for the new release has to be activated";
}

sub remedy_rtd_activate {
  my $settings = shift();

  die("Failed to set read the docs version to active") unless(rtd_api_version(
    $settings, 'PATCH',
    req_json_ref => {
      active => $JSON::PP::true,
      hidden => $JSON::PP::false,
    },
    expect_status => 204,
  ));
  return 1;
}

############################################################################

sub verify_trigger_shapes_demo_build {
  my $settings = shift();

  my $run = get_last_shapes_run($settings);
  if (!defined($run)) {
    print("No shapes demo build runs for $settings->{git_tag} found\n");
    return 0;
  }
  return 1;
}

sub message_trigger_shapes_demo_build {
  my $settings = shift();
  return "Shapes demo GitHub Action workflow has to be trigged";
}

sub remedy_trigger_shapes_demo_build {
  my $settings = shift();

  trigger_shapes_run($settings);

  print("Giving GitHub time to process...\n");
  sleep(5);

  return 1;
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
my $skip_sftp = 0;
my $skip_github = undef;
my $force_not_highest_version = 0;
my $sftp_base_dir = $default_sftp_base_dir;
my $upload_shapes_demo = 0;
my $update_authors = 0;

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
  'skip-sftp' => \$skip_sftp,
  'sftp-base-dir=s' => \$sftp_base_dir,
  'skip-github=i' => \$skip_github,
  'upload-shapes-demo' => \$upload_shapes_demo,
  'update-authors' => \$update_authors,
) or arg_error("Invalid option");

if ($print_help) {
  print help();
  exit(0);
}

if ($print_list_all) {
  $print_list = 1;
}

$mock = 1 if ($mock_with_doxygen);

if (defined($skip_github)) {
  $force_not_highest_version = $skip_github ? 0 : 1;
  $skip_github = 1;
}

my $workspace = "";
my $parsed_version;
my $parsed_next_version;
my $version = "";
my $base_name = "";
my $release_branch = "";

my $ignore_args = $update_authors || $print_list_all;
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

  if (!$skip_sftp) {
    die("SFTP_USERNAME, SFTP_HOST need to be defined")
      if (!(defined($ENV{SFTP_USERNAME}) && defined($ENV{SFTP_HOST})));
  }
}

my $release_timestamp = POSIX::strftime($release_timestamp_fmt, gmtime);
$release_timestamp =~ s/  / /g; # Single digit days of the month result in an extra space

my $doxygen_dir = "$workspace/doxygen";
my $doxygen_inner_dir = "$doxygen_dir/html/dds";
my $this_changelog = "docs/history/ChangeLog-$version";
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
    tgz_worktree => "$workspace/tgz/${base_name}",
    zip_worktree => "$workspace/zip/${base_name}",
    doxygen_dir => $doxygen_dir,
    doxygen_inner_dir => $doxygen_inner_dir,
    tar_src => "${base_name}.tar",
    tgz_src => "${base_name}.tar.gz",
    zip_src => "${base_name}.zip",
    md5_src => "${base_name}.md5",
    sha256_src => "${base_name}.sha256",
    zip_dox => "${base_name}-doxygen.zip",
    devguide_ver => "${base_name_prefix}$parsed_version->{series_string}.pdf",
    devguide_lat => "${base_name_prefix}latest.pdf",
    timestamp => $release_timestamp,
    git_url => "git\@github.com:${github_user}/${repo_name}.git",
    github_repo => $repo_name,
    github_token => $ENV{GITHUB_TOKEN},
    read_the_docs_token => $ENV{READ_THE_DOCS_TOKEN},
    sftp_user => $ENV{SFTP_USERNAME},
    sftp_host => $ENV{SFTP_HOST},
    sftp_base_dir => $sftp_base_dir,
    changelog => $this_changelog,
    modified => {
        "NEWS.md" => 1,
        "PROBLEM-REPORT-FORM" => 1,
        "VERSION.txt" => 1,
        "dds/Version.h" => 1,
        $this_changelog => 1,
    },
    skip_sftp => $skip_sftp,
    skip_devguide => $skip_devguide,
    skip_doxygen => $skip_doxygen,
    skip_website => $skip_website,
    workspace => $workspace,
    workspace_info_file_path => "$workspace/$workspace_info_filename",
    download_url => $download_url,
    ace_root => "$workspace/$ace_root",
    mock => $mock,
    dummy_doxygen => $mock && !$mock_with_doxygen,
    skip_github => $skip_github,
    force_not_highest_version => $force_not_highest_version,
);

if (!$ignore_args) {
  $global_settings{pithub} = new_pithub(\%global_settings);

  check_workspace(\%global_settings);

  if ($mock) {
    if (!$skip_github && $github_user eq $default_github_user) {
      die("--github-user can't be left to default when using --mock!");
    }

    if (!$skip_sftp && $download_url eq $default_download_url) {
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
    name => 'Update VERSION.txt File',
    verify => sub{verify_update_version_file(@_)},
    message => sub{message_update_version_file(@_)},
    remedy => sub{remedy_update_version_file(@_)},
    can_force => 1,
  },
  {
    name => 'Update Version.h File',
    verify => sub{verify_update_version_h_file(@_)},
    message => sub{message_update_version_h_file(@_)},
    remedy => sub{remedy_update_version_h_file(@_)},
    can_force => 1,
  },
  {
    name => 'Update PROBLEM-REPORT-FORM File',
    verify => sub{verify_update_prf_file(@_)},
    message => sub{message_update_prf_file(@_)},
    remedy => sub{remedy_update_prf_file(@_)},
    can_force => 1,
  },
  {
    name => 'Create ChangeLog File',
    verify => sub{verify_changelog(@_)},
    message => sub{message_changelog(@_)},
    remedy => sub{remedy_changelog(@_)},
    can_force => 1,
  },
  {
    name => 'Update AUTHORS File',
    verify => sub{verify_authors(@_)},
    message => sub{message_authors(@_)},
    remedy => sub{remedy_authors(@_)},
    can_force => 1,
  },
  {
    name => 'Add NEWS File Section',
    verify => sub{verify_news_file_section(@_)},
    message => sub{message_news_file_section(@_)},
    remedy => sub{remedy_news_file_section(@_)},
    can_force => 1,
  },
  {
    name => 'Update NEWS File Section',
    verify => sub{verify_update_news_file(@_)},
    message => sub{message_update_news_file(@_)},
    can_force => 1,
  },
  {
    name => 'Update NEWS File Section Timestamp',
    verify => sub{verify_news_timestamp(@_)},
    message => sub{message_news_timestamp(@_)},
    remedy => sub{remedy_news_timestamp(@_)},
    can_force => 1,
  },
  {
    name => 'Commit Release Changes',
    verify => sub{
      my $settings = shift();
      my $step_options = shift();
      return verify_git_status_clean($settings, {%{$step_options}, strict => 1});
    },
    message => sub{message_commit_git_changes(@_)},
    remedy => sub{remedy_git_status_clean(@_)},
  },
  {
    name => 'Create Tag',
    verify => sub{verify_git_tag(@_)},
    message => sub{message_git_tag(@_)},
    remedy => sub{remedy_git_tag(@_)},
  },
  {
    name => 'Verify Remote',
    verify => sub{verify_git_remote(@_)},
    message => sub{message_git_remote(@_)},
    skip => $global_settings{skip_github},
  },
  {
    name => 'Push Release Changes',
    prereqs => ['Verify Remote'],
    verify => sub{verify_git_changes_pushed(@_)},
    message => sub{message_git_changes_pushed(@_)},
    remedy => sub{remedy_git_changes_pushed(@_)},
    skip => $global_settings{skip_github},
  },
  {
    name => 'Create Release Branch',
    verify => sub{verify_create_release_branch(@_)},
    message => sub{message_create_release_branch(@_)},
    remedy => sub{remedy_create_release_branch(@_)},
    skip => !$global_settings{release_branch},
  },
  {
    name => 'Push Release Branch',
    prereqs => ['Create Release Branch', 'Verify Remote'],
    verify => sub{verify_push_release_branch(@_)},
    message => sub{message_push_release_branch(@_)},
    remedy => sub{remedy_push_release_branch(@_)},
    skip => !$global_settings{release_branch} || $global_settings{skip_github},
  },
  {
    name => 'Create Unix Release Worktree',
    prereqs => ['Create Tag'],
    verify => sub{release_archive_worktree_verify('tgz', @_)},
    message => sub{release_archive_worktree_message('tgz', @_)},
    remedy => sub{release_archive_worktree_remedy('tgz', @_)},
  },
  {
    name => 'Create Unix Release Archive',
    prereqs => ['Create Unix Release Worktree'],
    verify => sub{release_archive_verify('tgz', @_)},
    message => sub{release_archive_message('tgz', @_)},
    remedy => sub{release_archive_remedy('tgz', @_)},
  },
  {
    name => 'Create Windows Release Worktree',
    prereqs => ['Create Tag'],
    verify => sub{release_archive_worktree_verify('zip', @_)},
    message => sub{release_archive_worktree_message('zip', @_)},
    remedy => sub{release_archive_worktree_remedy('zip', @_)},
  },
  {
    name => 'Create Windows Release Archive',
    prereqs => ['Create Windows Release Worktree'],
    verify => sub{release_archive_verify('zip', @_)},
    message => sub{release_archive_message('zip', @_)},
    remedy => sub{release_archive_remedy('zip', @_)},
  },
  {
    name => 'Create MD5 Checksum File',
    prereqs => [
      'Create Unix Release Archive',
      'Create Windows Release Archive',
    ],
    verify => sub{verify_md5_checksum(@_)},
    message => sub{message_md5_checksum(@_)},
    remedy => sub{remedy_md5_checksum(@_)},
  },
  {
    name => 'Create sha256 Checksum File',
    prereqs => [
      'Create Unix Release Archive',
      'Create Windows Release Archive',
    ],
    verify => sub{verify_sha256_checksum(@_)},
    message => sub{message_sha256_checksum(@_)},
    remedy => sub{remedy_sha256_checksum(@_)},
  },
  {
    name => 'Download OCI ACE/TAO',
    verify => sub{verify_download_ace_tao(@_)},
    message => sub{message_download_ace_tao(@_)},
    remedy => sub{remedy_download_ace_tao(@_)},
    skip => $global_settings{skip_doxygen},
  },
  {
    name => 'Extract OCI ACE/TAO',
    prereqs => ['Download OCI ACE/TAO'],
    verify => sub{verify_extract_ace_tao(@_)},
    message => sub{message_extract_ace_tao(@_)},
    remedy => sub{remedy_extract_ace_tao(@_)},
    skip => $global_settings{skip_doxygen},
  },
  {
    name => 'Generate Doxygen',
    prereqs => ['Extract OCI ACE/TAO', 'Create Unix Release Worktree'],
    verify => sub{verify_gen_doxygen(@_)},
    message => sub{message_gen_doxygen(@_)},
    remedy => sub{remedy_gen_doxygen(@_)},
    skip => $global_settings{skip_doxygen},
  },
  {
    name => 'Create Doxygen Archive',
    prereqs => ['Generate Doxygen'],
    verify => sub{verify_zip_doxygen(@_)},
    message => sub{message_zip_doxygen(@_)},
    remedy => sub{remedy_zip_doxygen(@_)},
    skip => $global_settings{skip_doxygen},
  },
  {
    name => 'Download Devguide',
    verify => sub{verify_devguide(@_)},
    message => sub{message_devguide(@_)},
    remedy => sub{remedy_devguide(@_)},
    skip => $global_settings{skip_devguide},
  },
  {
    name => 'Upload to SFTP Site',
    verify => sub{verify_sftp_upload(@_)},
    message => sub{message_sftp_upload(@_)},
    remedy => sub{remedy_sftp_upload(@_)},
    skip => $global_settings{skip_sftp},
  },
  {
    name => 'Upload to GitHub',
    verify => sub{verify_github_upload(@_)},
    message => sub{message_github_upload(@_)},
    remedy => sub{remedy_github_upload(@_)},
    skip => $global_settings{skip_github},
  },
  {
    name => 'Release Website',
    verify => sub{verify_website_release(@_)},
    message => sub{message_website_release(@_)},
    remedy => sub{remedy_website_release(@_)},
    skip => $global_settings{skip_website} || $global_settings{skip_github},
  },
  {
    name => 'Record that the Release Occurred',
    verify => sub{verify_release_occurred_flag(@_)},
    message => sub{return "Release ouccured flag needs to be set"},
    remedy => sub{remedy_release_occurred_flag(@_)},
  },
  {
    name => 'Update NEWS for Post-Release',
    verify => sub{verify_news_template_file_section(@_)},
    message => sub{message_news_template_file_section(@_)},
    remedy => sub{remedy_news_template_file_section(@_)},
    post_release => 1,
  },
  {
    name => 'Update VERSION.txt for Post-Release',
    verify => sub{
      my $settings = shift();
      my $step_options = shift();
      return verify_update_version_file($settings, {%{$step_options}, post_release => 1});
    },
    message => sub{message_update_version_file(@_)},
    remedy => sub{remedy_update_version_file(@_, 1)},
    can_force => 1,
    post_release => 1,
  },
  {
    name => 'Update Version.h for Post-Release',
    verify => sub{
      my $settings = shift();
      my $step_options = shift();
      return verify_update_version_h_file($settings, {%{$step_options}, post_release => 1});
    },
    message => sub{message_update_version_h_file(@_)},
    remedy => sub{remedy_update_version_h_file(@_, 1)},
    can_force => 1,
    post_release => 1,
  },
  {
    name => 'Update PROBLEM-REPORT-FORM for Post-Release',
    verify => sub{
      my $settings = shift();
      my $step_options = shift();
      return verify_update_prf_file($settings, {%{$step_options}, post_release => 1});
    },
    message => sub{message_update_prf_file(@_, 1)},
    remedy => sub{remedy_update_prf_file(@_, 1)},
    can_force => 1,
    post_release => 1,
  },
  {
    name => 'Commit Post-Release Changes',
    verify => sub{
      my $settings = shift();
      my $step_options = shift();
      verify_git_status_clean($settings, {%{$step_options}, strict => 1});
    },
    message => sub{message_commit_git_changes(@_)},
    remedy => sub{remedy_git_status_clean(@_, 1)},
    post_release => 1,
  },
  {
    name => 'Push Post-Release Changes',
    prereqs => ['Verify Remote'],
    verify => sub{verify_git_changes_pushed(@_)},
    message => sub{message_git_changes_pushed(@_)},
    remedy => sub{remedy_git_changes_pushed(@_, 0)},
    post_release => 1,
    skip => $global_settings{skip_github},
  },
  {
    name => 'Activate Version on Read the Docs',
    verify => sub{verify_rtd_activate(@_)},
    message => sub{message_rtd_activate(@_)},
    remedy => sub{remedy_rtd_activate(@_)},
    post_release => 1,
    can_force => 1,
    skip => 1, # When this is ready, also uncomment READ_THE_DOCS_TOKEN in help
  },
  {
    name => 'Trigger Shapes Demo Build',
    verify => sub{verify_trigger_shapes_demo_build(@_)},
    message => sub{message_trigger_shapes_demo_build(@_)},
    remedy => sub{remedy_trigger_shapes_demo_build(@_)},
    post_release => 1,
    skip => $global_settings{skip_github} || !$global_settings{is_highest_version},
  },
  {
    name => 'Email DDS-Release-Announce list',
    verify => sub{verify_email_list(@_)},
    message => sub{message_email_dds_release_announce(@_)},
    remedy => sub{remedy_email_dds_release_announce(@_)},
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
  my %args = @_;
  my $run_count_ref = $args{counts}->{run};
  my $skip_count_ref = $args{counts}->{skip};
  my $fail_count_ref = $args{counts}->{failk};
  my $prereq_of = $args{prereq_of};

  my $step = $release_steps->[$step_number-1];
  my $title = $step->{name};

  if (!$settings->{list_all} && ($step->{skip} || $step->{verified} ||
      ($settings->{release_occurred} && !$step->{post_release}))) {
    ++${$skip_count_ref};
    return;
  }

  if ($prereq_of) {
    print(" A prereq. of \"$prereq_of\" is ");
  }
  print "$step_number: $title\n";
  return if $settings->{list};

  ++${$run_count_ref};

  # Check Prerequisite Steps
  if ($step->{prereqs}) {
    foreach my $prereq_name (@{$step->{prereqs}}) {
      run_step($settings, $release_steps, get_step_by_name($prereq_name), prereq_of => $title);
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

  my $prereq_insert = '';
  if ($prereq_of) {
    $prereq_insert = "Prereq. of \"$prereq_of\", ";
  }
  print " $prereq_insert Step $step_number \"$title\" is OK!\n";

  $step->{verified} = 1;
}

my $alt = $upload_shapes_demo || $update_authors;
if ($upload_shapes_demo) {
  upload_shapes_demo(\%global_settings);
}
elsif ($update_authors) {
  remedy_authors(\%global_settings);
}
elsif (!$alt && ($ignore_args || ($workspace && $parsed_version))) {
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
      counts => {run => \$run_count, skip => \$skip_count, fail => \$fail_count});
  }

  print("Ran: $run_count\n") if ($run_count);
  print("Skipped: $skip_count\n") if ($skip_count);
  print("Failed: $fail_count\n") if ($fail_count);
}
else {
  $status = 1;
  print STDERR "ERROR: Invalid arguments, see --help for valid usage\n";
}

exit $status;
