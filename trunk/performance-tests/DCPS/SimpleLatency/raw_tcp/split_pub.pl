eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
     & eval 'exec perl -S $0 $argv:q'
     if 0;

use Env (DDS_ROOT);
use lib "$DDS_ROOT/bin";
use lib "$ENV{ACE_ROOT}/bin";
use PerlDDS::Run_Test;

$PUB_1 = PerlDDS::create_process("raw_tcp_publisher",
                              "-p 192.168.1.2:9999 -n 10000 -d 9 -s 192.168.1.3:5555");

my $status = 0;

print "Launch Publisher 1...\n";
$PUB_1->Spawn();

my $status_pub_1 = $PUB_1->WaitKill(300);

if ($status_pub_1 != 0) {
    print "ERROR: publisher 1 returned $status_pub_1\n";
    $status = 1;
}


exit $status;
