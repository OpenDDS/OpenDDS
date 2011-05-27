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

my $subscriber = new PerlACE::Process('subscriber', "-p $port");
my $publisher  = new PerlACE::Process('publisher',  "-p $port");

unlink($dst, $sig);

$subscriber->Spawn();
if (PerlACE::waitforfile_timed($sig, 5) == -1) {
  print STDERR "ERROR: waiting for $sig\n";
  $subscriber->Kill();
  exit(1);
}
            
$publisher->SpawnWaitKill(10);
$subscriber->WaitKill(5);

if (-s $src != -s $dst || compare($src, $dst) != 0) {
  print "ERROR: $dst does not match $src\n";
}

unlink($dst, $sig);
exit(0);
