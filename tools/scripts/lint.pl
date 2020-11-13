#!/usr/bin/perl

# Linter for style and usage checks

use strict;
use warnings;

use File::Spec::Functions;
use File::Find qw/find/;
use Encode qw/decode encode FB_CROAK/;
use Cwd qw/abs_path/;
use Getopt::Long qw/GetOptions/;

my $line_length_limit = 100;

my $debug = 0;
my $ace = 1;
my $simple_output = 0;
my $files_only = 0;
my $help = 0;
my $list_checks = 0;
my $list_default_checks = 0;
my $list_non_default_checks = 0;
my $all = 0;
my $alt_root = '';

my $usage_message =
  "usage: lint.pl [-h|--help] | [OPTIONS] [CHECK ...]\n";

# TODO: Way to give arguments to ace fuzz.pl

my $help_message = $usage_message .
  "\n" .
  "OpenDDS Repo General Linter\n" .
  "\n" .
  "CHECK\t\t\tCheck(s) to run. If no checks are given, then the default checks are run.\n" .
  "\n" .
  "OPTIONS:\n" .
  "--help | -h\t\tShow this message\n" .
  "--debug\t\t\tPrint script debug information\n" .
  "--[no-]ace\t\tRun ACE's fuzz.pl? Requires ACE_ROOT. True by default\n" .
  "--alt-root PATH\tLook at files in PATH instead of DDS_ROOT\n" .
  "--simple-output\t\tPrint individual errors as single lines\n" .
  "--files-only\t\tJust print the files that failed\n" .
  "--list | -l\t\tList all checks\n" .
  "--list-default\t\tList all default checks\n" .
  "--list-non-default\tList all non-default checks\n" .
  "--all | -a\t\tRun all checks\n";

if (!GetOptions(
  'h|help' => \$help,
  'debug' => \$debug,
  'ace!' => \$ace,
  'alt-root=s' => \$alt_root,
  'simple-output' => \$simple_output,
  'files-only' => \$files_only,
  'l|list' => \$list_checks,
  'list-default' => \$list_default_checks,
  'list-non-default' => \$list_non_default_checks,
  'a|all' => \$all,
)) {
  print STDERR $usage_message;
  exit 1;
}
if ($help) {
  print $help_message;
  exit 0;
}

my $listing_checks = $list_checks || $list_default_checks ||  $list_non_default_checks;
$all = 1 if $list_checks;

if (not $listing_checks) {
  my @missing_env = ();
  push(@missing_env, 'DDS_ROOT') if (!defined $ENV{'DDS_ROOT'} && !$alt_root);
  push(@missing_env, 'ACE_ROOT') if (!defined $ENV{'ACE_ROOT'} && $ace);
  if (scalar(@missing_env)) {
    die(join(', ', @missing_env) . " must be defined!");
  }
}

my $opendds_checks_failed = 0;

my $root = $alt_root;
if (!$root) {
  $root = $ENV{'DDS_ROOT'};
}
my $dds_root_len = length($root);

sub is_elf_file {
  my $full_filename = shift;
  open(my $fd, $full_filename) or die("Could not open $full_filename: $!");
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
  open(my $fd, $full_filename) or die("Could not open $full_filename: $!");
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
  in_secattr_config => qr@^tests/security/attributes/(governance|permissions)@,
  in_modeling => qr@^tools/modeling@,
  in_old_bench => qr@^performance-tests/Bench@,
  in_java_jms => qr@^java/jms@,

  cpp_file => qr/\.(cpp|h|hpp|inl)$/,
  cpp_header_file => qr/\.(h|hpp|inl)$/,
  idl_file => qr/\.idl$/,
  cmake_file => qr/(CMakeLists\.txt|\.cmake)$/,
  md_file => qr/\.md$/,
  rst_file => qr/\.rst$/,
  p7s_file => qr/\.p7s$/,
  mpc_file => qr/\.(mpc|mwc)$/,
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
);

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
#   ignore => Array of specific file paths to ignore
# }
my %all_checks = (

  gettimeofday => {
    path_matches_all_of => ['cpp_file', 'in_dds_dcps'],
    line_matches => qr/gettimeofday/,
    message => [
      'ACE_OS::gettimeofday() and "ACE_Time_Value().now()" are forbidden in the core libraries.',
      'See the "Time" section in docs/guidelines.md.',
    ],
  },

  trailing_whitespace => {
    path_matches_all_of => [
      'text_file',
      '!in_secattr_config',
      '!in_modeling',
      '!in_old_bench',
      '!in_java_jms',
      '!p7s_file',

      '!make_file',
      '!tao_idl_gen_file',
      '!old_design_files',
      '!svg_file',
      '!batch_file',
    ],
    line_matches => qr/\s\n$/,
    message => [
      'Text file has trailing whitespace at the end of lines'
    ],
  },

  tabs => {
    path_matches_all_of => [
      'text_file',
      '!in_secattr_config',
      '!in_modeling',
      '!in_old_bench',
      '!in_java_jms',
      '!p7s_file',
      '!svg_file',

      '!make_file',
      '!old_design_files',
      '!mpc_file',
      '!batch_file',
    ],
    ignore => [
      'tests/DCPS/Crossplatform/test_list.txt',
      'tests/security/certs/identity/index.txt',
    ],
    line_matches => qr/\t/,
    message => [
      'Text file has tabs'
    ],
  },

  final_newline => {
    path_matches_all_of => [
      'text_file',
      '!in_secattr_config',
      '!in_modeling',
      '!in_old_bench',
      '!p7s_file',
    ],
    message => [
      'Text file must end with one and only one newline'
    ],
    file_matches => sub {
      my $filename = shift;
      my $full_filename = shift;
      my $line_numbers = shift;

      open(my $fd, $full_filename);
      my $count = 0;
      while (my $line = <$fd>) {
        if ($line =~ /^\n$/) {
          $count += 1;
        } else {
          $count = 0;
        }
      }
      close($fd);

      return $count == 1;
    }
  },

  line_length => {
    # Converts to UTF-32 for the proper length length, checking for valid UTF-8 along the way
    message => [
      'Text file either has extremely long lines or has invalid UTF-8'
    ],
    default => 0, # TODO: Make Default
    path_matches_any_of => ['cpp_file', 'idl_file'],
    line_matches => sub {
      my $line = shift;
      my $result;
      eval '$result = encode("UTF-32", decode("UTF-8", $line, FB_CROAK), FB_CROAK)';
      print("    decode says: $@") if ($@ && $debug);
      return $@ || (length($result) / 4) > $line_length_limit;
    },
  },

  utf8 => {
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
    ignore => [
      'tests/security/certs/permissions/index.txt',
      'performance-tests/Bench/tools/plot-all.gp',
    ],
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

  nonprefixed_public_macros => {
    message => [
      'Public macros must be prefixed with OPENDDS or ACE'
    ],
    default => 0, # TODO: Make Default
    path_matches_all_of => ['cpp_header_file', 'in_dds_dcps'],
    line_matches => sub {
      my $line = shift;
      if ($line =~ /#\s*define\s*(\w+)/) {
        my $what = $1;
        if (!($what =~ /^(OPENDDS|ACE)/)) {
          # print "$what\n";
          return 1;
        }
      }
      return 0;
    },
  },
);

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

if ($listing_checks) {
  foreach my $name (@checks) {
    print_check($name, *STDOUT);
  }
  exit 0;
}

sub process_path_condition {
  my $path_condition_expr = shift;
  my $filename = shift;
  my $full_filename = shift;

  if ($path_condition_expr !~ /^(\!)?(\w+)$/) {
    die("Invalid path condition: $path_condition_expr");
  }

  my $invert = $1;
  my $path_condition = $2;

  if (!defined $path_conditions{$path_condition}) {
    die("Invalid path condition: $path_condition_expr");
  }

  my $type = ref($path_conditions{$path_condition});
  my $val;
  if ($type eq "Regexp") {
    $val = $filename =~ $path_conditions{$path_condition};
  }
  elsif ($type eq "CODE") {
    $val = $path_conditions{$path_condition}->($filename, $full_filename);
  }
  else {
    die("Invalid type for $path_condition: $type");
  }
  if ($invert) {
    return !$val;
  } else {
    return $val;
  }
}

sub process_file {
  my $full_filename = $_;
  $full_filename = shift if (!defined $full_filename); # Needed for direct invoke for some reason...
  my $filename = substr($full_filename, $dds_root_len);
  $filename =~ s/^\///g;
  return if (!length $filename
    || $filename =~ qr@^\.git/@
    || $filename =~ qr@^\.gitmodules$@
    || $filename =~ qr@^tests/googletest/@
    || $filename =~ qr@^tools/rapidjson/@
    || $filename =~ qr@^ACE_wrappers@
    || $filename =~ qr@^ACE_TAO@
  );
  my %line_numbers = ();
  my $failed = 0;

  # Figure Out What Checks To Do For This File
  my @checks_for_this_file = ();
  foreach my $name (@checks) {
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
          return shift =~ $all_checks{$check}->{line_matches};
        };
      }
      elsif ($type eq "CODE") {
        $check_sub = $all_checks{$check}->{line_matches};
      }
      else {
        die("Invalid line_matches type for $check: $type");
      }

      open(my $fd, $full_filename);
      while (my $line = <$fd>) {
        if ($check_sub->($line)) {
          $failed = 1;
          if (defined $line_numbers{$check}) {
            my @line_numbers = @{$line_numbers{$check}};
            push(@line_numbers, $.);
            $line_numbers{$check} = \@line_numbers;
          } else {
            $line_numbers{$check} = [$.];
          }
        }
      }
      close($fd);
      $checked = 1;
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
              print STDERR ("ERROR: $filename:$ln failed $check check\n");
            }
          }
          else {
            print STDERR ("ERROR: $filename failed $check check\n");
          }
        }
      }
    }
    elsif ($files_only) {
      print "$filename\n";
    }
    else {
      print STDERR "ERROR: $filename has failed the following checks:\n";
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

find({wanted => \&process_file, follow => 0, no_chdir => 1}, $root);

# Run fuzz.pl (from ACE) passing in the list of tests applicable to OpenDDS

my $tests = join(',', qw/
check_for_inline_in_cpp
check_for_push_and_pop
check_for_ORB_init
check_for_refcountservantbase
/);

# not checking due to googletest and/or security:
# check_for_improper_main_declaration
# check_for_long_file_names

my $ace_result = 0;
if ($ace) {
  $ace_result = system(catfile($ENV{'ACE_ROOT'}, 'bin', 'fuzz.pl') . " -t $tests") >> 8;
}

exit(($ace_result || $opendds_checks_failed) ? 1 : 0);
