#!/usr/bin/perl

use strict;

use File::Spec::Functions;
use File::Find qw/find/;

my $opendds_checks_failed = 0;
my $dds_root_len = length($ENV{'DDS_ROOT'});

sub process_file {
  my $full_filename = $_;
  my $filename = substr($full_filename, $dds_root_len);
  my $cpp_source = $filename =~ /\.(cpp|h|inl)$/;
  my $in_dds_dcps = $filename =~ /^\/dds\/DCPS/;
  if (!($cpp_source && $in_dds_dcps)) {
    return;
  }

  my $gettimeofday = 0;

  open(my $fd, $full_filename);
  while (my $line = <$fd>) {
    if ($line =~ /gettimeofday|ACE_Time_Value\(\)\.now\(\)/) {
      $gettimeofday = 1;
    }
  }
  close($fd);

  my $failed_checks = "";
  if ($gettimeofday) {
    $failed_checks .=
      " - ACE_OS::gettimeofday() and \"ACE_Time_Value().now()\" are forbidden in the core libraries.\n" .
      "   Please use MonotonicTimePoint or SystemTimePoint defined in dds/DCPS/TimeTypes.h.\n";
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
