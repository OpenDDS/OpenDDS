eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# -*- perl -*-

use Sys::Hostname;

use Env (DDS_ROOT);
use lib "$DDS_ROOT/bin";
use Env (ACE_ROOT);
use lib "$ACE_ROOT/bin";
use PerlDDS::Run_Test;
use FileHandle;
use Cwd;
use strict;

my $single_test;

if(defined $ARGV[0])
{
  if($ARGV[0] ne '')
  {
    $single_test = "UnitTests_$ARGV[0]";
  }
}

my $something_ran = 0;
my $status = 0;

sub run_unit_tests {
  if ($single_test ne '') {
    my $test = new PerlDDS::TestFramework();
    $test->process($single_test, $single_test, "");
    $something_ran = 1;
    print STDERR "Running only $single_test\n";
    $test->start_process("$single_test");
    my $retcode = $test->finish(60);
    if ($retcode != 0) {
      $status = 1;
    }
  }
  else {
    my $dir = getcwd() . "/$PerlACE::Process::ExeSubDir";
    my $fh = new FileHandle();
    my $testExe = "^(UnitTests_[^\.]*?)(:?\.exe)?\$";
    if (opendir($fh, $dir)) {
      foreach my $file (readdir($fh)) {
        my $test = new PerlDDS::TestFramework();
        my $executable;
        if ($file =~ /$testExe/o) {
          $executable = $1;
          # each process runs to completion before the next starts
          my $LONE_PROCESS = 1;
          $test->process("$executable", "$executable", "", $LONE_PROCESS);
        }
        else {
          next;
        }
        $something_ran = 1;
        print STDOUT "Running $file\n";
        $test->start_process("$executable");
        my $retcode = $test->finish(60);
        if ($retcode != 0) {
          ++$status;
        }
      }
    }
    closedir($fh);
  }
}

run_unit_tests();

unless ($something_ran == 1) {
  print STDERR "ERROR: no test was run\n";
  exit 1;
}

exit $status;
