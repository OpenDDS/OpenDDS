eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
     & eval 'exec perl -S $0 $argv:q'
     if 0;

use Env (DDS_ROOT);
use lib "$DDS_ROOT/bin";
use lib "$ENV{ACE_ROOT}/bin";
use PerlDDS::Run_Test;

$SUB_1 = PerlDDS::create_process("raw_tcp_subscriber",
                              "-p 3 -n 2000 -d 8 -s 5555");
$SUB_2 = PerlDDS::create_process("raw_tcp_subscriber",
                              "-p 3 -n 2000 -d 8 -s 6555");
$SUB_3 = PerlDDS::create_process("raw_tcp_subscriber",
                              "-p 3 -n 2000 -d 8 -s 7555");
$PUB_1 = PerlDDS::create_process("raw_tcp_publisher",
                              "-p 1 -n 2000 -d 8 " .
                              "-s localhost:5555 " .
                              "-s localhost:6555 " .
                              "-s localhost:7555");
$PUB_2 = PerlDDS::create_process("raw_tcp_publisher",
                              "-p 2 -n 2000 -d 8 " .
                              "-s localhost:5555 " .
                              "-s localhost:6555 " .
                              "-s localhost:7555");
$PUB_3 = PerlDDS::create_process("raw_tcp_publisher",
                              "-p 3 -n 2000 -d 8 " .
                              "-s localhost:5555 " .
                              "-s localhost:6555 " .
                              "-s localhost:7555");

my $status = 0;

print "Launch Subscriber 1...\n";
$SUB_1->Spawn();
print "Launch Subscriber 2...\n";
$SUB_2->Spawn();
print "Launch Subscriber 3...\n";
$SUB_3->Spawn();
sleep 3;
print "Launch Publisher 1...\n";
$PUB_1->Spawn();
print "Launch Publisher 2...\n";
$PUB_2->Spawn();
print "Launch Publisher 3...\n";
$PUB_3->Spawn();

my $status_pub_1 = $PUB_1->WaitKill(300);
my $status_pub_2 = $PUB_2->WaitKill(300);
my $status_pub_3 = $PUB_3->WaitKill(300);
my $status_sub_1 = $SUB_1->WaitKill(5);
my $status_sub_2 = $SUB_2->WaitKill(5);
my $status_sub_3 = $SUB_3->WaitKill(5);

if ($status_pub_1 != 0) {
    print "ERROR: publisher 1 returned $status_pub_1\n";
    $status = 1;
}

if ($status_pub_2 != 0) {
    print "ERROR: publisher 2 returned $status_pub_2\n";
    $status = 1;
}

if ($status_pub_3 != 0) {
    print "ERROR: publisher 3 returned $status_pub_3\n";
    $status = 1;
}

if ($status_sub_1 != 0) {
    print "ERROR: subscriber 1 returned $status_sub_1\n";
    $status = 1;
}

if ($status_sub_2 != 0) {
    print "ERROR: subscriber 2 returned $status_sub_2\n";
    $status = 1;
}

if ($status_sub_3 != 0) {
    print "ERROR: subscriber 3 returned $status_sub_3\n";
    $status = 1;
}

exit $status;
