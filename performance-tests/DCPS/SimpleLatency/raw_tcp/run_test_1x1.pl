eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
     & eval 'exec perl -S $0 $argv:q'
     if 0;

use Env (DDS_ROOT);
use lib "$DDS_ROOT/bin";
use lib "$ENV{ACE_ROOT}/bin";
use PerlDDS::Run_Test;

$SUB_1 = PerlDDS::create_process("raw_tcp_subscriber",
                              "-n 4000 -d 9 -s 5555");
$PUB_1 = PerlDDS::create_process("raw_tcp_publisher",
                              "-n 4000 -d 9 -s localhost:5555");

my $status = 0;

print "Launch Subscriber 1...\n";
$SUB_1->Spawn();
sleep 5;
print "Launch Publisher 1...\n";
$PUB_1->Spawn();

my $status_pub_1 = $PUB_1->WaitKill(300);
my $status_sub_1 = $SUB_1->WaitKill(5);

if ($status_pub_1 != 0) {
    print "ERROR: publisher 1 returned $status_pub_1\n";
    $status = 1;
}

if ($status_sub_1 != 0) {
    print "ERROR: subscriber 1 returned $status_sub_1\n";
    $status = 1;
}

exit $status;
