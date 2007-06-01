eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
     & eval 'exec perl -S $0 $argv:q'
     if 0;

# $Id$
# -*- perl -*-

use Env (ACE_ROOT);
use lib "$ACE_ROOT/bin";
use PerlACE::Run_Test;
use File::Compare;

my $sig  = 'ready.txt';
my $src  = 'publisher.cpp';
my $dst  = 'data.txt';
my $port = PerlACE::random_port();

my $subscriber = new PerlACE::Process('subscriber', "-p $port -s $port");
my $publisher  = new PerlACE::Process('publisher',  "-p $port -s $port");

unlink($dst, $sig);

$subscriber->Spawn();
if (PerlACE::waitforfile_timed($sig, 5) == -1) {
  print STDERR "ERROR: waiting for $sig\n";
  $subscriber->Kill();
  exit(1);
}
            
$publisher->SpawnWaitKill(10);
$subscriber->WaitKill(5);

if (-s $dst == 0) {
  print "ERROR: Subscriber did not write any data\n";
}

unlink($dst, $sig);
exit(0);
