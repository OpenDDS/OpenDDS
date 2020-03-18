#!/usr/bin/perl

# Linter for style and usage checks

use strict;
use warnings;

use File::Spec::Functions;
use File::Find qw/find/;
use Encode qw/decode encode FB_CROAK/;
use Cwd qw/abs_path/;
use Getopt::Long qw/GetOptions/;

my $debug = 0;
my $ace_fuzz = 1;
my $simple_output = 0;
my $help = 0;

my $usage_message =
  "usage: dds_fuzz.pl [-h|--help] | [--debug] [--[no-]ace-fuzz] [--simple-output]\n";

# TODO: Way to define specific checks to do
# TODO: Way to give arguments to ace fuzz.pl

my $help_message = $usage_message .
  "\n" .
  "OpenDDS Source Code Linter\n";
  # TODO Document Arguments

my @extra_checks_array;
if (!GetOptions(
  'debug' => \$debug,
  'ace-fuzz!' => \$ace_fuzz,
  'simple-output' => \$simple_output,
  'h|help' => \$help,
)) {
  print STDERR $usage_message;
  exit 1;
}
if ($help) {
  print $help_message;
  exit 0;
}

my @missing_env = ();
push(@missing_env, 'DDS_ROOT') if !defined $ENV{'DDS_ROOT'};
push(@missing_env, 'ACE_ROOT') if (!defined $ENV{'ACE_ROOT'} && $ace_fuzz);
if (scalar(@missing_env)) {
  die(join(', ', @missing_env) . " must be defined!");
}

my %extra_checks = (); # TODO: Populate This

my $opendds_checks_failed = 0;
my $dds_root_len = length($ENV{'DDS_ROOT'});

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
  not_this_file => qr@^tools/scripts/dds_fuzz.pl$@,

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
  open_document_file => qr/\.od[gt]$/,
  excel_file => qr/\.xlam$/,
  photoshop_file => qr/\.psd$/,
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
#   extra => do not run this check unless told to explicitly
# }
my %checks = (

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
    ],
    line_matches => qr/\t/,
    message => [
      'Text file has tabs'
    ],
  },

  missing_final_newline => {
    path_matches_all_of => [
      'text_file',
      '!in_secattr_config',
      '!in_modeling',
      '!in_old_bench',
      '!in_java_jms',
      '!p7s_file',
      '!svg_file',
    ],
    message => [
      'Text file is missing an endline on the last line'
    ],
    file_matches => sub {
      my $filename = shift;
      my $full_filename = shift;
      my $last_line;
      open(my $fd, $full_filename);
      while (my $line = <$fd>) {
        $last_line = $line;
      }
      close($fd);
      if (defined $last_line) {
        return $last_line !~ /\n$/;
      }
      return 0;
    }
  },

  redundunt_final_newlines => {
    message => [
      'Text file has redundent newlines at the end'
    ],
    path_matches_all_of => [
      'text_file',
      '!in_modeling',
      '!p7s_file',
      '!in_old_bench',
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

      return $count > 0;
    }
  },

  unicode_line_length => {
    # Converts to UTF-32 for the proper length length, checking for valid UTF-8 along the way
    message => [
      'Text file either has extremely long lines or has invalid UTF-8'
    ],
    path_matches_any_of => ['cpp_file', 'idl_file'],
    line_matches => sub {
      my $line = shift;
      my $result;
      eval '$result = encode("UTF-32", decode("UTF-8", $line, FB_CROAK), FB_CROAK)';
      print("    decode says: $@") if ($@ && $debug);
      return $@ || (length($result) / 4) > 150; # TODO: Determine final limit to use
    },
  },

  is_empty => {
    message => [
      'File is empty'
    ],
    path_matches_all_of => ['empty_file'],
  },

  is_binary => {
    extra => 1,
    message => [
      'File looks like a binary file'
    ],
    path_matches_all_of => [
      'binary_file',
      '!jpg_file',
      '!png_file',
      '!jar_file',
      '!gif_file',
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
  );
  my %line_numbers = ();
  my $failed = 0;

  # Figure Out What Checks To Do For This File
  my @checks_for_this_file = ();
  while (my ($name, $check_ref) = each (%checks)) {
    my %check = %{$check_ref};

    # Skip Extra Tests Not Specified
    if (defined $check{extra} && $check{extra} && !exists $extra_checks{$name}) {
      next;
    }

    my $matched = 1;

    if (defined $check{path_matches_all_of}) {
      foreach my $path_condition (@{$check{path_matches_all_of}}) {
        if (!process_path_condition($path_condition, $filename, $full_filename)) {
          $matched = 0;
        }
      }
    }

    if (defined $check{path_matches_any_of}) {
      my $any_matched = 0;
      foreach my $path_condition (@{$check{path_matches_any_of}}) {
        if (process_path_condition($path_condition, $filename, $full_filename)) {
          $any_matched = 1;
        }
      }
      $matched = $matched && $any_matched;
    }

    if ($matched) {
      push(@checks_for_this_file, $name);
      $line_numbers{$name} = ();
    }
    if ($cmake_file && $line =~ /\t/) {
      push(@tabs_failed, $line_number);
    }
    $line_number += 1;
  }
  return if (not scalar @checks_for_this_file);

  # Do The Checks
  print("$filename: " . join(', ', @checks_for_this_file) . "\n") if $debug;
  foreach my $check (@checks_for_this_file) {
    my $checked = 0;

    if (defined $checks{$check}->{file_matches}) {
      print("  $check: file_matches\n") if $debug;
      $line_numbers{$check} = [];
      if ($checks{$check}->{file_matches}->($filename, $full_filename, \$line_numbers{$check})) {
        $failed = 1;
      } else {
        delete $line_numbers{$check};
      }
      $checked = 1;
    }

    if (-f $full_filename && defined $checks{$check}->{line_matches}) {
      print("  $check: line_matches\n") if $debug;
      my $type = ref($checks{$check}->{line_matches});
      my $check_sub;
      if ($type eq "Regexp") {
        $check_sub = sub {
          return shift =~ $checks{$check}->{line_matches};
        };
      }
      elsif ($type eq "CODE") {
        $check_sub = $checks{$check}->{line_matches};
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
    } else {
      print STDERR "ERROR: $filename has failed the following checks:\n";
      foreach my $check (@checks_for_this_file) {
        if (defined $line_numbers{$check}) {
          print STDERR ("  - $check\n");
          if (defined $checks{$check}->{message}) {
            my $indent = "    - ";
            foreach my $line (@{$checks{$check}->{message}}) {
              print STDERR ("$indent$line\n");
              $indent = "      ";
            }
          }
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
            print("\n");
          } else {
            print("    - (Applies to whole file)\n") if $debug;
          }
        }
      }
    }
    $opendds_checks_failed = 1;
  }
}

find({wanted => \&process_file, follow => 0, no_chdir => 1}, "$ENV{'DDS_ROOT'}");

# Run fuzz.pl (from ACE) passing in the list of tests applicable to OpenDDS

my $fuzz = catfile($ENV{'ACE_ROOT'}, 'bin', 'fuzz.pl');
my $tests = join(',', qw/
check_for_inline_in_cpp
check_for_push_and_pop
check_for_changelog_errors
check_for_ORB_init
check_for_refcountservantbase
/);

# not checking due to googletest and/or security:
# check_for_improper_main_declaration
# check_for_long_file_names

my $ace_fuzz_result = 0;
if ($ace_fuzz) {
  $ace_fuzz_result = system("$fuzz -t $tests @ARGV") >> 8;
}

exit(($ace_fuzz_result || $opendds_checks_failed) ? 1 : 0);
