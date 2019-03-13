#!/usr/bin/perl

# Run fuzz.pl (from ACE) passing in the list of tests applicable to OpenDDS

use File::Spec::Functions;
use strict;

if (!defined $ENV{"ACE_ROOT"})
{
  print "ERROR: ACE_ROOT environment variable not defined. Exiting... \n";
  exit 1;
}

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
check_for_id_string
/);

# not checking due to googletest and/or security:
# check_for_improper_main_declaration
# check_for_long_file_names
exit(system("$fuzz -t $tests @ARGV") >> 8);
