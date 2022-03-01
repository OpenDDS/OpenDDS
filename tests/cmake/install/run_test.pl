eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

use strict;
use warnings;

use Cwd qw(abs_path);
use File::Find qw(find);

use lib "$ENV{DDS_ROOT}/bin";
use lib "$ENV{ACE_ROOT}/bin";
use PerlDDS::Run_Test;

my $prefix = "the-install-prefix";
my $abs_prefix = abs_path($prefix);

PerlDDS::add_lib_path("$abs_prefix/lib");

my $test = new PerlDDS::TestFramework();
if ($test->flag('clean-env')) {
  # We need to run this in a clean environment to make sure it's just using
  # what's in the prefix. env -i omits all the current environment variables,
  # sh sets basic variables, and run_in_prefix.sh sets the variables for
  # using the prefix.
  exec('env', '-i', 'sh', 'run_in_prefix.sh', $abs_prefix, $^X, 'run_test.pl')
    or die("exec failed: $!");
}
else {
  print("Prefix is $abs_prefix\nThese are all the files in the prefix:\n");
  find({
    wanted => sub {
      print("  $_\n");
      # my $full_filename = abs_path($_);
    },
    follow => 0,
    no_chdir => 1,
  }, $abs_prefix);

  $test->process("user", "$prefix/bin/opendds_install_test_user");
  $test->start_process("user");
  exit($test->finish(2) ? 1 : 0);
}
