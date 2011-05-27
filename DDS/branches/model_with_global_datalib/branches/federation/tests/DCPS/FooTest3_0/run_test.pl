eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# $Id$
# -*- perl -*-

use Env (DDS_ROOT);
use lib "$DDS_ROOT/bin";
use Env (ACE_ROOT);
use lib "$ACE_ROOT/bin";
use DDS_Run_Test;

# Set the library path for the client to be able to load
# the FooTyoe* library.
PerlDDS::add_lib_path('../FooType3');

$status = 0;

# Configuration for default test - register test.
$test_to_run = 0;
$num_writes = 3;
$n_chunks = 20; # Number of pre-allocated chunks for Dynamic_Cached_Allocator
$shutdown_pub = 1;
$add_new_subscription = 0;
$num_subscribers = 1;
$shutdown_delay_secs=10;
$sub_ready_file = "sub_ready.txt";

$svc_config = new PerlACE::ConfigList->check_config ('STATIC') ? ''
    : "-ORBSvcConf ../../tcp.conf";

if ($ARGV[0] eq 'unregister') {
    $test_to_run = 1;
    $num_writes = 6;   # 1 register, 1 unregister, 1 reregister 3 success writes.
}
elsif ($ARGV[0] eq 'dispose') {
    $test_to_run = 2;
    $num_writes = 12;  # 1 register, 1 dispose and 10 writes.
}
elsif ($ARGV[0] eq 'resume') {
    $test_to_run = 3;
    $num_writes = 11;  # 1 register, 10 writes.
}
elsif ($ARGV[0] eq 'listener') {
    $test_to_run = 4;
}
elsif ($ARGV[0] eq 'allocator') {
    $test_to_run = 5;
    $n_chunks = 2;
    $num_writes = 5;  # 1 register, 4 writes - 2 allocate from pool and 2 allocate from heap.
}
elsif ($ARGV[0] eq 'liveliness') {
    $test_to_run = 6;
    $num_writes = 4;;  # 1 register, 3 writes
}

#Disabled the reenqueue_all test since we changed to not use the TransientKludge,
#as the condition of reenqueue_all, the TRANSIENT_LOCAL durability is supported.
#This reenqueue_all test will not be applicable in the new TRANSIENT_LOCAL
#implementation.

#elsif ($ARGV[0] eq 'reenqueue_all') { # transient_local support test
#    # 2 readers with default register test
#    # The first datareader does not shutdown the publisher
#    # and the second datareader will shutdown the publisher.
#
#    # Made the shutdown delay longer so the first subscriber
#    # will keep the connection with the publisher until the
#    # the publisher shutdown.
#    $shutdown_delay_secs=30;
#    $shutdown_pub = 0;
#    $num_subscribers = 2;
#
#    # The first reader will get 5 msgs in total
#    $num_writes = 5;
#}
elsif ($ARGV[0] eq '') { # register test
    # default register test: 1 register and 2 writes.
}
else {
    print STDERR "ERROR: invalid parameter $ARGV[0] \n";
    exit 1;
}

$dcpsrepo_ior = "dcps_ir.ior";
$pubdriver_ior = "pubdriver.ior";
# The pub_id_fname can not be a full path because the
# pub_id_fname will be part of the parameter of the -p option
# which will be parsed using ':' delimiter.
$pub_id_fname = "pub_id.txt";

$pub_port = PerlACE::random_port();
$sub_port = PerlACE::random_port();
$sub_id = 1;
$history_depth=10;
$repo_bit_conf = "-NOBITS";
$app_bit_conf = "-DCPSBit 0";

unlink $dcpsrepo_ior;
unlink $pub_id_fname;
unlink $pubdriver_ior;
unlink $sub_ready_file;

$DCPSREPO = PerlDDS::create_process ("$ENV{DDS_ROOT}/bin/DCPSInfoRepo",
                                    "$svc_config $repo_bit_conf -o $dcpsrepo_ior ");
print $DCPSREPO->CommandLine(), "\n";

$publisher = PerlDDS::create_process ("FooTest3_publisher",
                                     "$svc_config "
                                     . "$app_bit_conf -p $pub_id_fname:localhost:$pub_port "
                                     . "-s $sub_id:localhost:$sub_port "
                                     . " -DCPSInfoRepo file://$dcpsrepo_ior -d $history_depth"
                                     . " -t $test_to_run -DCPSChunks $n_chunks -v $pubdriver_ior"
                                     . " -f $sub_ready_file");

print $publisher->CommandLine(), "\n";

$subscriber = PerlDDS::create_process ("FooTest3_subscriber",
                                      "$svc_config "
                                      . "-p $pub_id_fname:localhost:$pub_port "
                                      . "$app_bit_conf -s $sub_id:localhost:$sub_port -n $num_writes "
                                      . "-v file://$pubdriver_ior -x $shutdown_pub "
                                      . "-a $add_new_subscription -d $shutdown_delay_secs "
                                      . "-f $sub_ready_file");

print $subscriber->CommandLine(), "\n";


$DCPSREPO->Spawn ();

if (PerlACE::waitforfile_timed ($dcpsrepo_ior, 30) == -1) {
    print STDERR "ERROR: waiting for DCPSInfo IOR file\n";
    $DCPSREPO->Kill ();
    exit 1;
}


$subscriber->Spawn ();
$publisher->Spawn ();


if ($num_subscribers == 2)
{
    sleep (5);

    $shutdown_delay_secs=10;
    $sub_id = 2;
    $sub_port = 7777;
    $add_new_subscription = 1;
    $num_writes = 2; # 2 writes

    $subscriber2 = PerlDDS::create_process ("FooTest3_subscriber",
                                           "$svc_config "
                                           . "-p $pub_id_fname:localhost:$pub_port "
                                           . "-s $sub_id:localhost:$sub_port -n $num_writes "
                                           . "-v file://$pubdriver_ior -x 1 -a 1 "
                                           . "-d $shutdown_delay_secs -f $sub_ready_file");

    print $subscriber2->CommandLine(), "\n";

    $subscriber2->Spawn ();

    $result = $subscriber2->WaitKill(60);

    if ($result != 0) {
	print STDERR "ERROR: $subscriber2 returned $result  \n";
	$status = 1;
    }
}


$result = $subscriber->WaitKill(60);
if ($result != 0) {
    print STDERR "ERROR: $subscriber returned $result  \n";
    $status = 1;
}

$result = $publisher->WaitKill (60);
if ($result != 0) {
    print STDERR "ERROR: $publisher returned $result \n";
    $status = 1;
}

$ir = $DCPSREPO->TerminateWaitKill(5);
if ($ir != 0) {
    print STDERR "ERROR: DCPSInfoRepo returned $ir\n";
    $status = 1;
}

exit $status;
