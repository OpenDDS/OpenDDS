eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
     & eval 'exec perl -S $0 $argv:q'
     if 0;

# -*- perl -*-

use lib "$ENV{ACE_ROOT}/bin";
use lib "$ENV{DDS_ROOT}/bin";
use PerlDDS::Run_Test;
use strict;

my $port = PerlACE::random_port();
my $subready = 'subready.txt';
unlink $subready;

my $SUB = PerlDDS::create_process('subscriber', "-h localhost -p $port");
my $result = $SUB->Spawn();
if ($result != 0) {
  print STDERR "ERROR: spawning subscriber returned $result\n";
  exit 1;
}
if (PerlACE::waitforfile_timed($subready, 10) == -1) {
  print STDERR "ERROR: waiting for subscriber file\n";
  $SUB->Kill();
  exit 1;
}

my $PUB = PerlDDS::create_process('publisher', "-h localhost -p $port");
$result = $PUB->SpawnWaitKill(60);
if ($result != 0) {
  print STDERR "ERROR: publisher returned $result\n";
}

$result = $SUB->WaitKill(60);
if ($result != 0) {
  print STDERR "ERROR: subscriber returned $result\n";
}

exit $result;
