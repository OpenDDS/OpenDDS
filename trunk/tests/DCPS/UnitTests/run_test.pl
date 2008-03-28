eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# $Id$
# -*- perl -*-

use Sys::Hostname;

use Env (ACE_ROOT);
use lib "$ACE_ROOT/bin";
use PerlACE::Run_Test;
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
  my $dir = getcwd();
  my $fh = new FileHandle();
  my $unixTestExe = "^UnitTests[^\.]*\$";
  my $windowsTestExe = "^UnitTests.*\.exe\$";
  if($single_test ne '') {
    $TST = new PerlACE::Process("$dir/$single_test", "");
    $something_ran = 1;
    print STDERR "Running only $single_test\n";
    my $retcode = $TST->SpawnWaitKill(60);
    if ($retcode != 0) {
      $status = 1;
    }
  }
  elsif (opendir($fh, $dir)) {
    foreach my $file (readdir($fh)) {
      my $TST;
      if ($file =~ /$unixTestExe/o) {
        $TST = new PerlACE::Process("$dir/$file", "");
      }
      elsif ($file =~ /$windowsTestExe/o) {
        $exename = "$dir/$file";
        $exename =~ s/\.exe$//i;
        $TST = new PerlACE::Process("$exename", "");
      }
      else {
        next;
      }
      $something_ran = 1;
      print STDOUT "Running $file\n";
      my $retcode = $TST->SpawnWaitKill(60);
      if ($retcode != 0) {
        ++$status;
      }
    }
    closedir($fh);
  }
}

run_unit_tests();


if (($status == 0) &&
    ($something_ran == 1)) {
  print "test PASSED.\n";
}
else {
  print STDERR "test FAILED.\n";
}

exit $status;
