eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
     & eval 'exec perl -S $0 $argv:q'
     if 0;

# -*- perl -*-

use lib "$ENV{ACE_ROOT}/bin";
use lib "$ENV{DDS_ROOT}/bin";
use PerlDDS::Run_Test;
use strict;

PerlDDS::add_lib_path('../TestMsg');

sub do_test {
  my $mcast = shift;
  my $port = PerlACE::random_port();
  my $subready = 'subready.txt';
  unlink $subready;

  my $test = new PerlDDS::TestFramework();
  my $sublog = "sub". ($mcast ? "_multicast" : "") . ".log";

  $test->process('subscriber', 'subscriber', "-h 127.0.0.1 -p $port -ORBLogFile $sublog");
  $test->start_process('subscriber');
  if (PerlACE::waitforfile_timed($subready, 30) == -1) {
    print STDERR "ERROR: waiting for subscriber file\n";
    $test->finish(1);
    return 1;
  }

  $test->process('publisher', 'publisher', $mcast ? "-h 239.255.0.2 -p 7401" : "-h 127.0.0.1 -p $port");
  $test->start_process('publisher');

  my $failed = $test->finish(60) ? 1 : 0;
  if ($failed) {
    print STDERR "ERROR: test failed\n";
  }
  return $failed;
}

my $failed = 0;
print "\nTesting best-effort readers...\n";
$failed |= do_test(0);
print "\nTesting best-effort readers, multicast...\n";
$failed |= do_test(1);

exit $failed;
