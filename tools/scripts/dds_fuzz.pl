#!/usr/bin/perl

use strict;

use File::Spec::Functions;
use File::Find qw/find/;

my $opendds_checks_failed = 0;
my $dds_root_len = length($ENV{'DDS_ROOT'});

sub failed_lines {
  my $lines_ref = shift;
  return "   - On the following lines: " . join(', ', @{$lines_ref}) . "\n";
}

sub process_file {
  my $full_filename = $_;
  my $filename = substr($full_filename, $dds_root_len);
  my $line_number = 1;

  my $in_dds_dcps = $filename =~ /^\/dds\/DCPS/;
  my $cpp_file = $filename =~ /\.(cpp|h|inl)$/;
  my $cmake_file = $filename =~ /(CMakeLists\.txt|\.cmake)$/;

  my @gettimeofday_failed = ();
  my @trailing_whitespace_failed = ();
  my @tabs_failed = ();

  open(my $fd, $full_filename);
  while (my $line = <$fd>) {
    if ($cpp_file && $in_dds_dcps && $line =~ /gettimeofday|ACE_Time_Value\(\)\.now\(\)/) {
      push(@gettimeofday_failed, $line_number);
    }
    if ($cmake_file && $line =~ /\s\n$/) {
      push(@trailing_whitespace_failed, $line_number);
    }
    if ($cmake_file && $line =~ /\t/) {
      push(@tabs_failed, $line_number);
    }
    $line_number += 1;
  }
  close($fd);

  my $failed_checks = "";
  if (scalar @gettimeofday_failed) {
    $failed_checks .=
      " - ACE_OS::gettimeofday() and \"ACE_Time_Value().now()\" are forbidden in the core libraries.\n" .
      "   See the \"Time\" section in docs/guidelines.md.\n" .
      failed_lines(\@gettimeofday_failed);
  }
  if (scalar @trailing_whitespace_failed) {
    $failed_checks .=
      " - Text file has trailing whitespace, which is forbidden in all text files by docs/guidelines.md\n" .
      failed_lines(\@trailing_whitespace_failed);
  }
  if (scalar @tabs_failed) {
    $failed_checks .=
      " - Text file has tabs, which is forbidden in most text files by docs/guidelines.md\n" .
      failed_lines(\@tabs_failed);
  }

  if (length($failed_checks)) {
    print STDERR "$filename has failed the following checks:\n$failed_checks";
    $opendds_checks_failed = 1;
  }
}

find({wanted => \&process_file, follow => 0, no_chdir => 1}, $ENV{'DDS_ROOT'});

# Run fuzz.pl (from ACE) passing in the list of tests applicable to OpenDDS

my $fuzz = catfile($ENV{'ACE_ROOT'}, 'bin', 'fuzz.pl');
my $tests = join(',', qw/
check_for_newline
check_for_tab
check_for_inline_in_cpp
check_for_push_and_pop
check_for_changelog_errors
check_for_ORB_init
check_for_refcountservantbase
check_for_trailing_whitespace
/);

# not checking due to googletest and/or security:
# check_for_improper_main_declaration
# check_for_long_file_names

exit((system("$fuzz -t $tests @ARGV") >> 8) || $opendds_checks_failed);
