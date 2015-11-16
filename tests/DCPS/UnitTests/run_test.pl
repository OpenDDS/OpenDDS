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

if(defined $ARGV[0])
{
  if($ARGV[0] ne '')
  {
    $single_test = "UnitTests_$ARGV[0]";
  }
}

my $something_ran = 0;

sub run_unit_tests {
  if($single_test ne '') {
    my $test = new PerlDDS::TestFramework();
    $test->process("$single_test", "$single_test", "");
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
        if ($file =~ /$testExe/o) {
          my $executable = $1;
          # each process runs to completion before the next starts
          my $LONE_PROCESS = 1;
          if ($executable eq "UnitTests_BIT_DataReader") {
            $test->process ("$executable", "$executable", "-DCPSConfigFile rtps.ini", $LONE_PROCESS);
          } else {
            $test->process ("$executable", "$executable", "", $LONE_PROCESS);
          }
        }
        else {
          next;
        }
        $something_ran = 1;
        print STDOUT "Running $file\n";
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
}

exit $status;
