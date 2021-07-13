#!/usr/bin/perl

# Linter for style and usage checks

use strict;
use warnings;

use File::Spec::Functions;
use File::Basename;
use File::Find qw/find/;
use Encode qw/decode encode FB_CROAK/;
use Cwd qw/abs_path/;
use Getopt::Long qw/GetOptions/;
use Term::ANSIColor;

use constant line_length_limit => 100;
use constant eof_newline_limit => 2;

STDOUT->autoflush(1);

my $debug = 0;
my $ace = 1; # Warn if ACE not found
my $ace_arg = 0; # Error if ACE not found
my $no_ace_arg = 0; # Silence warning
my @paths = ();
my @ext_paths = ();
my $simple_output = 0;
my $files_only = 0;
my $help = 0;
my $list_checks = 0;
my $list_default_checks = 0;
my $list_non_default_checks = 0;
my $all = 0;
my $fix = 0;
my @includes = ();
my $isatty = -t STDOUT && -t STDERR;
my $color = $isatty;
my $no_color_arg = 0;
my $color_arg = 0;

# Term::ANSIColor doesn't seem to give real control over color, so provide our
# own.
sub colour {
  my $rv = '';
  if ($color) {
    foreach my $arg (@_) {
      $rv .= color($arg);
    }
  }
  return $rv;
}

sub colour_else {
  my $not_color = shift;
  if ($color) {
    return colour(@_);
  }
  else {
    return $not_color;
  }
}

sub mark_error {
  my $s = shift;
  $s =~ s/\t/        /g;
  return colour_else('>>>', 'white', 'on_red') .  $s . colour_else('<<<', 'reset');
}

sub error {
  return colour('red') . "ERROR:" . colour('reset');
}

sub print_error {
  print STDERR (error() . " " . shift . "\n");
}

sub print_warning {
  print STDERR (colour('yellow') . "WARNING:" . colour('reset') . " " . shift . "\n");
}

sub print_note {
  print(colour('blue') . "NOTE" . colour('reset') . " " . shift . "\n");
}

# Try to Find ACE_ARGS
my @ace_args = ();
my @saved_argv = ();
my $found_ace_args = 0;
while (my $arg = shift(@ARGV)) {
  if ($found_ace_args) {
    push(@ace_args, $arg);
  }
  elsif ($arg eq '--') {
    $found_ace_args = 1;
  }
  else {
    push(@saved_argv, $arg);
  }
}
@ARGV = @saved_argv;

my $usage_message =
  "usage: lint.pl [-h|--help] | [OPTIONS] [CHECK ...] [-- ACE_ARGS ...]\n";

#  ###############################################################################
my $help_message = $usage_message .
  "\n" .
  "OpenDDS Repo General Linter\n" .
  "\n" .
  "CHECK               Check(s) to run. If no checks are given, then the default\n" .
  "                    checks are run.\n" .
  "ACE_ARGS            Arguments to pass to ACE's fuzz.pl\n" .
  "\n" .
  "OPTIONS:\n" .
  "--help | -h         Show this message\n" .
  "--debug             Print script debug information\n" .
  "--[no-]ace          Run ACE's fuzz.pl? Requires ACE to be in a usual place or\n" .
  "                    ACE_ROOT to be defined. By default the script will try to\n" .
  "                    use ACE, but issue a warning if it can't be found. Pass\n" .
  "                    --no-ace to silence this warning. Pass --ace to turn the\n" .
  "                    warning into an error if ACE can't be found.\n" .
  "--path | -p PATH    Restrict to PATH instead of everything in OpenDDS. Can be\n" .
  "                    a directory or a file. Can be specified multiple times.\n" .
  "                    PATH must be relative to the root of OpenDDS.\n" .
  "--ext-path PATH     Path outside the OpenDDS source tree to check.\n" .
  "                    Can be specified multiple times.\n" .
  "--simple-output     Print individual errors as single lines\n" .
  "--files-only        Just print the files that failed\n" .
  "--list | -l         List all checks\n" .
  "--list-default      List all default checks\n" .
  "--list-non-default  List all non-default checks\n" .
  "--all | -a          Run all checks\n" .
  "--try-fix           ATTEMPT to fix issues that support fixing. THIS IS POWERED\n" .
  "                    BY REGEX, NOT MAGIC. Don't try this unless your existing\n" .
  "                    work is committed or otherwise safe from this script.\n" .
  "                    See --list to see what checks support this.\n" .
  "--include PATH      Add PATH to preproccessor include paths for the sake of\n" .
  "                    checks that care about them. Can be specified multiple\n" .
  "                    times. Default is root of OpenDDS source tree.\n" .
  "--[no-]color        Force enable or disable ANSI escape code color output.\n" .
  "                    Can also be done with NO_COLOR environment variable.\n" .
  "                    By default it uses isatty like most programs\n" .
  "\n" .
  "If run with DDS_ROOT being defined, it will use that path. If not it will\n" .
  "use the OpenDDS source tree the script is in.\n" .
  "\n" .
  "The script can be configured to ignore checks in two ways. First way is by\n" .
  "comment. This takes following form:\n" .
  "  lint.pl ignores CHECK_NAME... on next line\n" .
  "This only works with checks that are checked on a line by line basis,\n" .
  "which are indicated by --list.\n" .
  "The second method is using .lint_config files. These are files with lines\n" .
  "that take the form:\n" .
  "  ignore [check[s] CHECK_NAME...] path[s] PATH...\n" .
  "check and path can optionally be plural for readability.\n" .
  "If there are no checks then the entire file is ignored. If PATH is \".\", then\n" .
  "all the files in the directory are ignored except the .lint_config file itself.\n" .
  "For comments and .lint_config files the checks and paths are space separated.\n";
#  ###############################################################################

if (!GetOptions(
  'h|help' => \$help,
  'debug' => \$debug,
  'ace' => \$ace_arg,
  'no-ace' => \$no_ace_arg,
  'path=s' => \@paths,
  'ext-path=s' => \@ext_paths,
  'simple-output' => \$simple_output,
  'files-only' => \$files_only,
  'l|list' => \$list_checks,
  'list-default' => \$list_default_checks,
  'list-non-default' => \$list_non_default_checks,
  'a|all' => \$all,
  'try-fix' => \$fix,
  'include=s' => \@includes,
  'color' => \$color_arg,
  'no-color' => \$no_color_arg,
)) {
  print STDERR $usage_message;
  exit 1;
}
if ($help) {
  print $help_message;
  exit 0;
}

if ($no_color_arg) {
  $color = 0;
}
elsif ($color_arg) {
  $color = 1;
  # Override these because we don't have a way to tell Term::ANSIColor to
  # ignore them.
  if (defined($ENV{NO_COLOR}) || defined($ENV{ANSI_COLORS_DISABLED})) {
    undef($ENV{NO_COLOR});
    undef($ENV{ANSI_COLORS_DISABLED});
  }
}
elsif (defined($ENV{NO_COLOR}) || defined($ENV{ANSI_COLORS_DISABLED})) {
  $color = 0;
}

my $listing_checks = $list_checks || $list_default_checks ||  $list_non_default_checks;
$all = 1 if $list_checks;

my $root = $ENV{'DDS_ROOT'} || dirname(dirname(dirname(abs_path(__FILE__))));
# Make sure root has / at the end to make string ops eaiser.
$root =~ s@(?<!/)$@/@;
push(@includes, "$root");
my $root_re = quotemeta($root);

$ace = 0 if ($no_ace_arg && !$ace_arg);
my $ace_root = $ENV{'ACE_ROOT'};
if ($ace and not $listing_checks and !defined $ace_root) {
  my @possible_ace_roots = (
    catfile($root, 'ACE_TAO', 'ACE'),
    catfile($root, 'ACE_wrappers'),
  );
  foreach my $possible_ace_root (@possible_ace_roots) {
    if (-d $possible_ace_root) {
      $ace_root = $possible_ace_root;
    }
  }
  if ($ace and !defined $ace_root) {
    my $common = "ACE_ROOT wasn't defined and we couldn't find ACE on our own.";
    if ($ace_arg) {
      die("${\error()} $common This is a fatal error since --ace was passed.");
    }
    print_warning("$common Not running ACE's fuzz.pl. " .
      "Pass --no-ace to silence this warning.");
    $ace = 0;
  }
}

if (scalar(@paths) == 0 && scalar(@ext_paths) == 0) {
  push(@paths, $root);
}
else {
  my @full_paths;
  foreach my $path (@paths) {
    my $full_path = catfile($root, $path);
    die("${\error()} $path does not exist in DDS_ROOT") if (not -e $full_path);
    push(@full_paths, $full_path);
  }
  @paths = @full_paths;
}

foreach my $path (@ext_paths) {
  push(@paths, $path);
}

sub is_elf_file {
  my $full_filename = shift;
  open(my $fd, $full_filename) or die("${\error()} Could not open $full_filename: $!");
  binmode($fd);
  my $is_elf = 0;
  my $b;
  if (read($fd, $b, 4) == 4) {
    $is_elf = $b eq "\x7f\x45\x4c\x46";
  }
  close($fd);
  return $is_elf;
}

sub is_empty_file {
  my $full_filename = shift;
  open(my $fd, $full_filename) or die("${\error()} Could not open $full_filename: $!");
  binmode($fd);
  my $b;
  my $eof = read($fd, $b, 1) == 0;
  close($fd);
  return $eof;
}

sub is_binary_file {
  my $full_filename = shift;
  return (-B $full_filename && !is_empty_file($full_filename)) || is_elf_file($full_filename);
}

use constant MATCH_ALL => '__match_all__';

# <name> => <regex to match to path> or <sub(filename, full_filename)>
my %path_conditions = (
  not_this_file => qr@^tools/scripts/opendds_link.pl$@,

  dir => sub {
    my $filename = shift;
    my $full_filename = shift;
    return -d $full_filename;
  },
  file => sub {
    my $filename = shift;
    my $full_filename = shift;
    return -f $full_filename;
  },
  binary_file => sub {
    my $filename = shift;
    my $full_filename = shift;
    return -f $full_filename && is_binary_file($full_filename);
  },
  text_file => sub {
    my $filename = shift;
    my $full_filename = shift;
    return -f $full_filename && !is_binary_file($full_filename);
  },
  empty_file => sub {
    my $filename = shift;
    my $full_filename = shift;
    return -f $full_filename && is_empty_file($full_filename);
  },

  in_dds_dcps => qr@^dds/DCPS@,
  in_core_includes => qr@^dds/(?!(idl|InfoRepo)/)@,

  cpp_header_file => qr/\.(h|hpp)$/,
  cpp_inline_file => qr/\.inl$/,
  cpp_public_file => ['cpp_header_file', 'cpp_inline_file'],
  cpp_source_file => qr/\.cpp$/,
  cpp_file => ['cpp_public_file', 'cpp_source_file'],
  idl_file => qr/\.idl$/,
  cmake_file => qr/(CMakeLists\.txt|\.cmake)$/,
  md_file => qr/\.md$/,
  rst_file => qr/\.rst$/,
  p7s_file => qr/\.p7s$/,
  mpc_file => qr/\.(mpc|mpb|mwc)$/,
  make_file => qr/((GNUm|M)akefile|\.make$)/,
  tao_idl_gen_file => qr/^.+(C|S).(h|cpp)$/,
  old_design_files => qr/\.(unt|uml|xmi|dia|asta)$/,
  svg_file => qr/\.svg$/,
  jpg_file => qr/\.jpe?g$/,
  png_file => qr/\.png$/,
  jar_file => qr/\.jar$/,
  gif_file => qr/\.gif$/,
  ico_file => qr/\.ico$/,
  open_document_file => qr/\.od[gt]$/,
  excel_file => qr/\.xlam$/,
  photoshop_file => qr/\.psd$/,
  batch_file => qr/\.(cmd|bat)$/,

  preprocessor_file => [
    'cpp_file',
    'idl_file',
  ],
  source_code => [
    'preprocessor_file',
    'cmake_file',
    qr/\.(py|pl|rb|java)$/,
  ],
  export_header => sub {
    my $filename = shift;
    my $full_filename = shift;

    if (!-f $full_filename || $filename !~ /export.*\.h$/i) {
      return 0;
    }

    my $is_export = 0;
    open(my $fd, $full_filename);
    while (my $line = <$fd>) {
      if ($line =~ /This file is generated automatically by generate_export_file\.pl/) {
        $is_export = 1;
        last;
      }
    }
    close($fd);

    return $is_export;
  },
  needs_include_guard => [
    MATCH_ALL,
    'in_core_includes',
    '!export_header',
    [
      'cpp_header_file',
      'idl_file',
    ],
  ],
  user_preprocessor_scope => [
    MATCH_ALL,
    'in_core_includes',
    [
      'cpp_public_file',
      'idl_file',
    ],
  ],
);

sub valid_include_guard_names {
  my @list;
  my $x = shift;
  $x =~ s/^dds\///g;
  $x =~ s/\W+/_/g;
  push(@list, $x);
  $x =~ s/([a-z])([A-Z])/$1_$2/g;
  push(@list, $x);
  return map {'OPENDDS_' . uc($_)} @list;
}

sub startswith {
  my $str = shift;
  my $prefix = shift;
  return rindex($str, $prefix, 0) == 0;
}

sub match_prefix_get_suffix {
  my $str = shift;
  my $prefix = shift;
  my $suffix;
  if (startswith($str, $prefix)) {
    $suffix = substr($str, length($prefix));
  }
  return $suffix;
}

# <name> => {
#   path_matches_all_of => <array ref of names of path_conditions>
#    - File is matched for check if all these path conditions are met
#   path_matches_any_of => <array ref of names of path_conditions>
#    - File is matched for conditions if at least one of these path conditions are met
#    - If path_matches_all_of is also specified, then they both must match the path.
#   file_matches => sub(filename, full_filename, line_numbers)
#     - Called once for the file once matched. If it returns true, then the file failed the check.
#   line_matches => <regex of the line to match a failure> or
#                   <sub(line) where if it returns true, it failed the check>
#     - Ran for every line
#     - If file_matches and line_matches are not defined the file always fails the check.
#   message => Array of strings to print when a failure happens
#     - If left out no message is printed
#   default => do not run this check unless told to explicitly. default is true.
#   strip_fix => If line_matched is being used with a regex with capture groups
#     around error and it can be fixed by simply omitted the contents of the
#     capture groups, allow --try-fix to do that.
#   can_fix => Simply indicate that this check **could** be fixed by running
#     --try-fix. This just inserts a messsage during --list.
# }
my %all_checks = (

  gettimeofday => {
    path_matches_all_of => ['cpp_file', 'in_dds_dcps'],
    line_matches => qr/(gettimeofday|ACE_Time_Value\(\)\.now\(\))/,
    message => [
      'ACE_OS::gettimeofday() and "ACE_Time_Value().now()" are forbidden in the core libraries.',
      'See the "Time" section in docs/guidelines.md.',
    ],
  },

  ace_condition => {
    path_matches_all_of => ['cpp_file', 'in_dds_dcps'],
    line_matches => qr/(?:ACE_|ace\/)Condition(?:(?:_Recursive)?_Thread_Mutex)?/,
    message => [
      'Except for in ConditionVariable.h, ACE_Condition and related types are forbidden in the core libraries.',
      'See the "Time" section in docs/guidelines.md.',
    ],
  },

  trailing_whitespace => {
    path_matches_all_of => [
      'text_file',
      '!p7s_file',
      '!make_file',
      '!tao_idl_gen_file',
      '!old_design_files',
      '!svg_file',
      '!batch_file',
    ],
    line_matches => qr/(\s+)\n$/,
    message => [
      'Text file has trailing whitespace at the end of lines'
    ],
    strip_fix => 1,
  },

  tabs => {
    path_matches_all_of => [
      'text_file',
      '!p7s_file',
      '!svg_file',
      '!make_file',
      '!old_design_files',
      '!mpc_file',
      '!batch_file',
    ],
    line_matches => qr/(\t)/,
    message => [
      'Text file has tabs'
    ],
  },

  eof_newline_count => {
    path_matches_all_of => [
      'text_file',
      '!p7s_file',
    ],
    message => [
      "Test file must end in at least one newline, but not more than " .
      "${\eof_newline_limit}",
    ],
    file_matches => sub {
      my $filename = shift;
      my $full_filename = shift;
      my $line_numbers = shift;

      open(my $fd, $full_filename);
      my $count = 0;
      while (my $line = <$fd>) {
        if ($line =~ /^(.*)?\n$/) {
          if ($1) {
            $count = 1;
          }
          else {
            $count += 1;
          }
        }
        else {
          $count = 0;
        }
      }
      close($fd);

      return $count == 0 || $count > eof_newline_limit;
    }
  },

  line_length => {
    # Converts to UTF-32 for the proper length length, checking for valid UTF-8 along the way
    message => [
      "Source code either has a line longer than ${\line_length_limit} Unicode " .
      "code points or has invalid UTF-8",
    ],
    default => 0, # TODO: Make Default
    path_matches_any_of => ['source_code'],
    line_matches => sub {
      my $line = shift;
      my $result;
      eval '$result = encode("UTF-32", decode("UTF-8", $line, FB_CROAK), FB_CROAK)';
      print("    decode says: $@") if ($@ && $debug);
      return $@ || (length($result) / 4) > line_length_limit;
    },
  },

  invalid_utf8 => {
    # Asserts that all text files are valid UTF-8. Will always be really slow,
    # so it's not a default check.
    # TODO: Refactor with line_length and don't run this when line_length will
    # also run.
    message => [
      'Text file has invalid UTF-8'
    ],
    default => 0,
    path_matches_all_of => ['text_file'],
    line_matches => sub {
      my $line = shift;
      eval 'encode("UTF-32", decode("UTF-8", $line, FB_CROAK), FB_CROAK)';
      print("    decode says: $@") if ($@ && $debug);
      return $@;
    },
  },

  is_empty => {
    message => [
      'File is empty'
    ],
    path_matches_all_of => ['empty_file'],
  },

  is_binary => {
    message => [
      'File looks like a binary file'
    ],
    path_matches_all_of => [
      'binary_file',
      '!jpg_file',
      '!png_file',
      '!jar_file',
      '!gif_file',
      '!ico_file',
      '!open_document_file',
      '!old_design_files',
      '!excel_file',
      '!photoshop_file',
    ],
  },

  path_has_whitespace => {
    message => [
      'Path has whitespace',
    ],
    path_matches_all_of => [qr/\s/],
  },

  whitespace_before_newline_in_string => {
    message => [
      'Whitespace before a newline in a string'
    ],
    path_matches_all_of => ['source_code'],
    line_matches => qr/".*(\s+)\\n.*"/,
    strip_fix => 1,
  },

  nonprefixed_public_macros => {
    message => [
      'Public macros must be prefixed with OPENDDS'
    ],
    default => 0, # TODO: Make Default
    path_matches_all_of => ['user_preprocessor_scope'],
    line_matches => sub {
      my $line = shift;
      if ($line =~ /^\s*#\s*define\s+(\w+)/) {
        my $what = $1;
        if ($what !~ /^OPENDDS/i && $what !~ /^ACE_ALLOCATOR/ && $what ne 'ACE_HAS_TRACE') {
          print("$what\n");
          return 1;
        }
      }
      return 0;
    },
  },

  missing_include_guard => {
    message => [
      'File is missing include guard with correct name',
    ],
    can_fix => 1,
    path_matches_all_of => ['needs_include_guard'],
    file_matches => sub {
      my $filename = shift;
      my $full_filename = shift;
      my $line_numbers = shift;

      my @valid_names = valid_include_guard_names($filename);
      my @lines;
      my $ifndef = '';
      my $fixed = 0;
      open(my $fd, $full_filename) or die("${\error()} $filename: $!");
      while (my $line = <$fd>) {
        if (length($ifndef)) {
          if ($line =~ /^#(\s*)define (\w+)$/) {
            my $indent = $1;
            my $define = $2;
            if ($define eq $ifndef) {
              foreach my $valid_name (@valid_names) {
                if ($ifndef eq $valid_name) {
                  close($fd);
                  return 0;
                }
              }
              # print("$ifndef\n");
              # foreach my $valid_name (@valid_names) {
              #   print("$valid_name\n");
              # }
              if ($fix) {
                pop(@lines);
                my $new_guard_name = $valid_names[0];
                push(@lines, "#${indent}ifndef $new_guard_name\n");
                $line = "#${indent}define $new_guard_name\n";
                $fixed = 1;
              }
            }
          }
          $ifndef = '';
        }
        if (length($ifndef) == 0) {
          if ($line =~ /^#\s*ifndef (\w+)$/) {
            $ifndef = $1;
          }
        }
        if ($fix) {
          push(@lines, $line);
        }
      }
      close($fd);
      if ($fix) {
        if ($fixed) {
          write_for_fix($filename, $full_filename, 'missing_include_guard', \@lines);
        } else {
          print_warning("Tried to fix include guard for $filename, but it " .
            "doesn't have an existing include guard to modify");
        }
      }
      return 1;
    }
  },

  'nonrelative_include_path' => {
    message => [
      'Path for include is non-relative when it should be relative',
    ],
    can_fix => 1,
    # TODO: Get https://github.com/DOCGroup/ACE_TAO/pull/1357 and
    # https://github.com/DOCGroup/ACE_TAO/pull/1366 merged before this can be
    # enforced on IDL files.
    # path_matches_all_of => ['preprocessor_file'],
    path_matches_all_of => ['cpp_file'],
    file_matches => sub {
      my $filename = shift;
      my $full_filename = shift;
      my $line_numbers = shift;
      my ($vol, $dir, $file) = File::Spec->splitpath($full_filename);

      my @included_in = ();
      foreach my $include_path (@includes) {
        my $suffix = match_prefix_get_suffix($dir, $include_path);
        if (defined($suffix)) {
          $suffix =~ s@^/@@;
          push(@included_in, $suffix);
        }
      }

      my $failed = 0;
      my @lines = ();
      open(my $fd, $full_filename);
      while (my $line = <$fd>) {
        my $line_failed = 0;
        if ($line =~ /^(\s*#\s*include\s*(?:\/\*\s*\*\/\s*)?)["<]([^"]*)[">](.*)$/) {
          my $before = $1;
          my $what = $2;
          my $after = $3;
          foreach my $find (@included_in) {
            my $suffix = match_prefix_get_suffix($what, $find);
            if (defined($suffix)) {
              $failed = 1;
              $line_failed = 1;
              push(@{${$line_numbers}}, $.);
              if ($fix) {
                push(@lines, "$before\"$suffix\"$after\n");
              }
            }
          }
        }
        if ($fix && !$line_failed) {
          push(@lines, $line);
        }
      }
      close($fd);
      if ($failed && $fix) {
        write_for_fix($filename, $full_filename, 'nonrelative_include_path', \@lines);
      }

      return $failed;
    },
  },
);

sub match_line {
  my $line = shift;
  my $regex = shift;

  my @parts = ();
  my $matched = 0;
  my $index;
  my $after_last_error = 0;
  while ($line =~ m/$regex/g) {
    if (scalar @{^CAPTURE}) {
      my $i = 0;
      foreach my $g (@{^CAPTURE}) {
        $index = $-[$i + 1];
        my $preerror = $index - $after_last_error;
        push(@parts, substr($line, $after_last_error, $preerror));
        push(@parts, $g);
        $i += 1;
        $after_last_error = $index + length($g);
      }
    }
    $matched = 1;
  }
  if (@parts) {
    $index = length($line);
    push(@parts, substr($line, $after_last_error, $index - $after_last_error));
  }
  return $matched, \@parts;
}

sub write_for_fix {
  my $filename = shift;
  my $full_filename = shift;
  my $check = shift;
  my $lines = shift;

  print_note("writing fixed version of $filename for $check");
  open(my $fd, '>', $full_filename) or die("${\error()} $filename: $!");
  foreach my $line (@{$lines}) {
    print $fd $line;
  }
  close($fd);
}

sub print_check {
  my $check = shift;
  my $file = shift;
  print $file ("  - $check\n");
  if (defined $all_checks{$check}->{message}) {
    my $indent = "    - ";
    foreach my $line (@{$all_checks{$check}->{message}}) {
      print $file ("$indent$line\n");
      $indent = "      ";
    }
  }
}

my @checks = ();
if ($all) {
  @checks = keys(%all_checks);
}
elsif (scalar(@ARGV)) {
  @checks = @ARGV;
  foreach my $name (@checks) {
    if (!exists $all_checks{$name}) {
      die("${\error()} Invalid check: $name");
    }
  }
}
else { # Default
  foreach my $name (keys(%all_checks)) {
    my %check = %{$all_checks{$name}};
    if ((defined $check{default} && !$check{default}) != $list_non_default_checks) {
      next;
    }
    push(@checks, $name);
  }
}
@checks = sort(@checks);

if ($listing_checks) {
  foreach my $name (@checks) {
    print_check($name, *STDOUT);
    if (defined $all_checks{$name}->{line_matches}) {
      print(
        "    - Can be disabled using a comment containing:\n" .
        "        lint.pl ignores $name on next line\n");
    }
    if (defined $all_checks{$name}->{default} && !$all_checks{$name}->{default}) {
      print(
        "    - Disabled by default\n");
    }
    if ((defined $all_checks{$name}->{can_fix} && $all_checks{$name}->{can_fix}) ||
        (defined $all_checks{$name}->{strip_fix} && $all_checks{$name}->{strip_fix})) {
      print(
        "    - May be fixed by running with --try-fix\n");
    }
  }
  exit 0;
}

sub process_path_condition_array {
  my $filename = shift;
  my $full_filename = shift;
  my $array = shift;

  my $match_all = 0;
  my $val = 0;
  foreach my $element (@{$array}) {
    if ($element eq MATCH_ALL) {
      $match_all = 1;
      $val = 1;
    }
    else {
      my $result = process_path_condition($element, $filename, $full_filename);
      if ($match_all) {
        if (!$result) {
          $val = 0;
          last;
        }
      }
      elsif ($result) {
        $val = 1;
        last;
      }
    }
  }
  return $val;
}

sub process_path_condition {
  my $path_condition_expr = shift;
  my $filename = shift;
  my $full_filename = shift;

  my $ref_type = ref($path_condition_expr);
  if ($ref_type eq "Regexp") {
    return $filename =~ $path_condition_expr;
  }
  elsif ($ref_type eq "ARRAY") {
    return process_path_condition_array(
      $filename, $full_filename, $path_condition_expr);
  }

  if ($path_condition_expr !~ /^(\!)?(\w+)$/) {
    die("${\error()} Invalid path condition: $path_condition_expr");
  }

  my $invert = $1;
  my $path_condition = $2;

  if (!defined $path_conditions{$path_condition}) {
    die("${\error()} Invalid path condition: $path_condition_expr");
  }

  $ref_type = ref($path_conditions{$path_condition});
  my $val;
  if ($ref_type eq "Regexp") {
    $val = $filename =~ $path_conditions{$path_condition};
  }
  elsif ($ref_type eq "CODE") {
    $val = $path_conditions{$path_condition}->($filename, $full_filename);
  }
  elsif ($ref_type eq "ARRAY") {
    $val = process_path_condition_array(
      $filename, $full_filename, $path_conditions{$path_condition});
  }
  else {
    die("${\error()} Invalid type for $path_condition: $ref_type");
  }

  if ($invert) {
    return !$val;
  } else {
    return $val;
  }
}

my %directories_seen;
my %paths_seen;
my %paths_to_ignore;

sub check_lint_config {
  my $full_dir_path = shift;
  my $full_lint_config = catfile($full_dir_path, '.lint_config');
  return if (not -f $full_lint_config);
  my $dir_path = shift;
  $dir_path =~ s/^\///g;
  my $lint_config = catfile($dir_path, '.lint_config');
  $lint_config =~ s/^\///g;
  print("Config: $lint_config\n") if $debug;

  open(my $fd, $full_lint_config) or die("${\error()} Could not open $full_lint_config $!");
  while (my $line = <$fd>) {
    $line =~ s/\n$//g;
    $line =~ s/#.*$//g;
    $line =~ s/^\s+$//g;
    next if (length($line) == 0);
    my @parts = split(/ /, $line);
    my $what = shift(@parts);
    if ($what eq 'ignore') {
      $what = shift(@parts);

      # The optional checks first. If not specified, all checks are ignored.
      my $checks_to_ignore = [];
      if ($what =~ /^check?$/) {
        while (my $check = shift(@parts)) {
          if ($check =~ /^paths?$/) {
            $what = $check;
            last;
          }
          if (!exists($all_checks{$check})) {
            die("${\error()} $full_lint_config:$.: Invalid check: \"$check\"");
          }
          push(@{$checks_to_ignore}, $check);
        }
      }

      # Required paths. "." means the entire directory is ignored.
      if ($what =~ /^paths?$/) {
        if (scalar(@parts)) {
          while (my $path = shift(@parts)) {
            my $whole_dir = $path eq '.';
            if ($whole_dir) {
              print("  ignoring this directory\n") if $debug;
              $paths_to_ignore{$full_dir_path} = $checks_to_ignore;
              return 1;
            }
            my $full_path = catfile($full_dir_path, $path);
            my $short_path = catfile($dir_path, $path);
            if ($debug) {
              if (scalar(@{$checks_to_ignore})) {
                print("  ignore checks ", join(', ', @{$checks_to_ignore}), " in $short_path\n");
              }
              else {
                print("  ignore $short_path\n");
              }
            }
            $paths_to_ignore{$full_path} = $checks_to_ignore;
          }
        }
      }
      else {
        die("${\error()} $full_lint_config:$.: Unexpected word: \"$what\"");
      }
    }
    else {
      die("${\error()} $full_lint_config:$.: Unexpected word: \"$what\"");
    }
    die("${\error()} $full_lint_config:$.: Leftover words") if (scalar(@parts));
  }
  close($fd);

  return 0;
}

sub process_directory {
  my $full_dir_path = $File::Find::dir;
  my $dir_path = $full_dir_path;
  $dir_path =~ s/$root_re//;
  my @dir_contents = sort(@_);

  print("process_directory: $dir_path\n") if($debug);

  if (exists($directories_seen{$full_dir_path})) {
    print("  (already seen)\n") if ($debug);
    return ();
  }
  $directories_seen{$full_dir_path} = 1;

  if (exists($paths_to_ignore{$full_dir_path}) and
      scalar(@{$paths_to_ignore{$full_dir_path}}) == 0) {
    print("  (ignored)\n") if ($debug);
    return ('.lint_config');
  }

  if (check_lint_config($full_dir_path, $dir_path)) {
    return ('.lint_config');
  }

  return @dir_contents;
}

my $opendds_checks_failed = 0;
my %checks_map = map {$_ => 1} @checks;
sub process_path {
  my $full_filename = $_;
  my $filename = $full_filename;
  $filename =~ s/$root_re//;

  print("process_path: $filename\n") if ($debug);

  if (!-r $full_filename) {
    print("  (doesn't exist)\n") if ($debug);
    return;
  }

  if (exists($paths_seen{$full_filename})) {
    print("  (already seen)\n") if ($debug);
    return;
  }
  $paths_seen{$full_filename} = 1;

  # Ignore file if it's ignored by git.
  my $cmd = "git check-ignore --quiet $full_filename";
  my $exit_status = 0;
  if (system($cmd)) {
    $exit_status = $? >> 8;
    if ($? == -1 || $? & 127 || $exit_status != 1) {
      die("${\error()} \"$cmd\" failed: $? $! $exit_status");
    }
  }
  # See https://git-scm.com/docs/git-check-ignore#_exit_status
  if ($exit_status == 0) {
    print("  (ignored by git)\n") if ($debug);
    return;
  }

  my @possible_checks_array = ();
  if (exists($paths_to_ignore{$full_filename})) {
    my @checks_to_ignore_array = @{$paths_to_ignore{$full_filename}};
    if (scalar(@checks_to_ignore_array) == 0) {
      print("  (ignored)\n") if ($debug);
      return;
    }
    my %checks_to_ignore_map = map {$_ => 1} @checks_to_ignore_array;
    foreach my $check (@checks) {
      if (!exists($checks_to_ignore_map{$check})) {
        push(@possible_checks_array, $check);
      }
    }
  }
  else {
    @possible_checks_array = @checks;
  }

  my %line_numbers = ();
  my $failed = 0;

  # Figure Out What Checks To Do For This Path
  my @checks_for_this_file = ();
  foreach my $name (@possible_checks_array) {
    my %check = %{$all_checks{$name}};
    my $matched = 1;

    if (defined $check{path_matches_all_of}) {
      foreach my $path_condition (@{$check{path_matches_all_of}}) {
        if (!process_path_condition($path_condition, $filename, $full_filename)) {
          $matched = 0;
          last;
        }
      }
    }

    if (defined $check{path_matches_any_of}) {
      my $any_matched = 0;
      foreach my $path_condition (@{$check{path_matches_any_of}}) {
        if (process_path_condition($path_condition, $filename, $full_filename)) {
          $any_matched = 1;
          last;
        }
      }
      $matched = $matched && $any_matched;
    }

    if (defined $check{ignore}) {
      foreach my $path (@{$check{ignore}}) {
        if ($filename eq $path) {
          $matched = 0;
          last;
        }
      }
    }

    if ($matched) {
      push(@checks_for_this_file, $name);
      $line_numbers{$name} = ();
    }
  }
  return if (not scalar @checks_for_this_file);

  # Do The Checks
  print("$filename: " . join(', ', @checks_for_this_file) . "\n") if $debug;
  foreach my $check (@checks_for_this_file) {
    my $fix_file = $fix &&
      defined $all_checks{$check}->{strip_fix} && $all_checks{$check}->{strip_fix};
    my $checked = 0;

    if (defined $all_checks{$check}->{file_matches}) {
      print("  $check: file_matches\n") if $debug;
      $line_numbers{$check} = [];
      if ($all_checks{$check}->{file_matches}->($filename, $full_filename, \$line_numbers{$check})) {
        $failed = 1;
      } else {
        delete $line_numbers{$check};
      }
      $checked = 1;
    }

    if (-f $full_filename && defined $all_checks{$check}->{line_matches}) {
      print("  $check: line_matches\n") if $debug;
      my $type = ref($all_checks{$check}->{line_matches});
      my $check_sub;
      if ($type eq "Regexp") {
        $check_sub = sub {
          my ($failed, $parts) = match_line(shift, $all_checks{$check}->{line_matches});
          my $marked_line = "";
          if ($failed) {
            my $mark = 0;
            foreach my $part (@{$parts}) {
              $marked_line .= $mark ? mark_error($part) : $part;
              $mark = !$mark;
            }
          }
          $marked_line =~ s/\n$//g;
          my $fixed_line = undef;
          if ($failed && $fix_file) {
            my $good = 1;
            foreach my $part (@{$parts}) {
              if ($good) {
                $fixed_line .= $part;
              }
              $good = !$good;
            }
          }
          return $failed, $marked_line, $fixed_line;
        };
      }
      elsif ($type eq "CODE") {
        $check_sub = $all_checks{$check}->{line_matches};
      }
      else {
        die("${\error()} Invalid line_matches type for $check: $type");
      }

      # TODO: Read each file just once for line_matches checks
      my @lines;
      my $expect_failure = 0;
      my $check_failed = 0;
      open(my $fd, $full_filename);
      while (my $line = <$fd>) {
        my $added_line = 0;
        my ($line_failed, $marked_line, $fixed_line) = $check_sub->($line);
        if ($line_failed and !$expect_failure) {
          $failed = 1;
          $check_failed = 1;
          if (defined $line_numbers{$check}) {
            my @line_numbers = @{$line_numbers{$check}};
            push(@line_numbers, $.);
            $line_numbers{$check} = \@line_numbers;
          } else {
            $line_numbers{$check} = [$.];
          }
          if (length $marked_line) {
            print_error("$filename:$.: $marked_line");
            if ($fix_file && defined($fixed_line)) {
              push(@lines, $fixed_line);
              $added_line = 1;
            }
          }
        }
        elsif (!$line_failed and $expect_failure) {
          die "${\error()} $filename:$.: $check was expected to fail on this line";
        }
        $expect_failure = 0;

        if ($line =~ /lint\.pl ignores (\w+(?: \w+)*) on next line/) {
          foreach my $skip_check_name (split(/ /, $1)) {
            if (!defined $all_checks{$skip_check_name}) {
              die "${\error()} $full_filename:$.: $skip_check_name is not a valid check";
            }
            if ($skip_check_name eq $check) {
              $expect_failure = 1;
              last;
            }
          }
        }

        if ($fix_file && !$added_line) {
          push(@lines, $line);
        }
      }
      close($fd);
      $checked = 1;

      if ($check_failed && $fix_file) {
        write_for_fix($filename, $full_filename, $check, \@lines);
      }
    }

    if (!$checked) {
      $failed = 1;
      $line_numbers{$check} = [];
    }
  }

  # Print Results
  if ($failed) {
    if ($simple_output) {
      foreach my $check (@checks_for_this_file) {
        if (defined $line_numbers{$check}) {
          if (scalar @{$line_numbers{$check}}) {
            foreach my $ln (@{$line_numbers{$check}}) {
              print_error("$filename:$ln failed $check check");
            }
          }
          else {
            print_error("$filename failed $check check");
          }
        }
      }
    }
    elsif ($files_only) {
      print "$filename\n";
    }
    else {
      print_error("$filename has failed the following checks:");
      foreach my $check (@checks_for_this_file) {
        if (defined $line_numbers{$check}) {
          print_check($check, *STDERR);
          if (scalar @{$line_numbers{$check}}) {
            print STDERR ("    - Failed on line(s): ");
            # Turn the list into a nice set of ranges
            my $last = 0;
            my $count = 0;
            foreach my $ln (@{$line_numbers{$check}}) {
              if ($count == 0) {
                $last = $ln;
                $count = 1;
                print STDERR ("$ln");
              }
              elsif (($ln - $last) == 1) {
                $last = $ln;
                $count += 1;
              }
              else {
                if ($count > 2) {
                  print STDERR (" - $last");
                }
                elsif ($count > 1) {
                  print STDERR (", $last");
                }
                print STDERR (", $ln");
                $last = $ln;
                $count = 1;
              }
            }
            if ($count > 2) {
              print STDERR (" - $last");
            }
            elsif ($count > 1) {
              print STDERR (", $last");
            }
            print STDERR ("\n");
          } else {
            print STDERR ("    - (Applies to the whole file)\n") if $debug;
          }
        }
      }
    }
    $opendds_checks_failed = 1;
  }
}

print_note("Running OpenDDS Lint Checks: " . join(' ', @checks));

foreach my $path (@paths) {
  find({
    preprocess => \&process_directory,
    wanted => \&process_path,
    follow => 0,
    no_chdir => 1,
  }, $path);
}

# Run fuzz.pl from ACE passing in the list of tests applicable to OpenDDS
my $ace_result = 0;
if ($ace) {
  if (!$found_ace_args) {
    my $tests = join(',', qw/
      check_for_inline_in_cpp
      check_for_push_and_pop
      check_for_ORB_init
      check_for_refcountservantbase
      /);
    # not checking due to googletest and/or security:
    # check_for_improper_main_declaration
    # check_for_long_file_names
    @ace_args = ('-t', $tests);
  }
  my $ace_args = join(' ', @ace_args);
  print_note("Running ACE Lint Checks: " . $ace_args);
  $ace_result = system(catfile($ENV{'ACE_ROOT'}, 'bin', 'fuzz.pl') . ' ' .  $ace_args) >> 8;
}

exit(($ace_result || $opendds_checks_failed) ? 1 : 0);
