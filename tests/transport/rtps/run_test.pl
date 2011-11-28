eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
     & eval 'exec perl -S $0 $argv:q'
     if 0;

# -*- perl -*-

use lib "$ENV{ACE_ROOT}/bin";
use lib "$ENV{DDS_ROOT}/bin";
use PerlDDS::Run_Test;
use strict;

sub do_test {
  my $mcast = shift;
  my $port = PerlACE::random_port();
  my $subready = 'subready.txt';
  unlink $subready;

  my $SUB = PerlDDS::create_process('subscriber', "-h localhost -p $port");
  my $result = $SUB->Spawn();
  print $SUB->CommandLine() . "\n";
  if ($result != 0) {
    print STDERR "ERROR: spawning subscriber returned $result\n";
    return 1;
  }
  if (PerlACE::waitforfile_timed($subready, 10) == -1) {
    print STDERR "ERROR: waiting for subscriber file\n";
    $SUB->Kill();
    return 1;
  }

  my $PUB = PerlDDS::create_process('publisher',
              $mcast ? "-h 239.255.0.2 -p 7401" : "-h localhost -p $port");
  $result = $PUB->SpawnWaitKill(60);
  print $PUB->CommandLine() . "\n";
  if ($result != 0) {
    print STDERR "ERROR: publisher returned $result\n";
  }

  $result = $SUB->WaitKill(60);
  if ($result != 0) {
    print STDERR "ERROR: subscriber returned $result\n";
  }

  return 0;
}

my $result = do_test(0);

print "Running with multicast...\n";
$result += do_test(1);

exit $result;
