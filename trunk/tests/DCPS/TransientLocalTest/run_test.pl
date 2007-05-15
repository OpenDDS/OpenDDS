eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# $Id$
# -*- perl -*-

use Env (ACE_ROOT);
use lib "$ACE_ROOT/bin";
use PerlACE::Run_Test;

$status = 0;

$opts =  "-ORBSvcConf tcp.conf";
$pub_opts = "$opts -DCPSConfigFile pub.ini";
$sub_opts = "$opts -DCPSConfigFile sub.ini";

$domains_file = PerlACE::LocalFile ("domain_ids");
$dcpsrepo_ior = PerlACE::LocalFile ("repo.ior");
$repo_bit_opt = "-ORBSvcConf tcp.conf";

unlink $dcpsrepo_ior;

$data_file = PerlACE::LocalFile ("test_run.data");
unlink $data_file;

$DCPSREPO = new PerlACE::Process ("$ENV{DDS_ROOT}/bin/DCPSInfoRepo",
				  "$repo_bit_opt -o $dcpsrepo_ior -d $domains_file");
$Subscriber = new PerlACE::Process ("subscriber", "$sub_opts");
$Publisher = new PerlACE::Process ("publisher", "$pub_opts " .
                                   "-ORBLogFile $data_file");

print $DCPSREPO->CommandLine() . "\n";
print $Publisher->CommandLine() . "\n";
print $Subscriber->CommandLine() . "\n";

$DCPSREPO->Spawn ();
if (PerlACE::waitforfile_timed ($dcpsrepo_ior, 30) == -1) {
    print STDERR "ERROR: waiting for DCPSInfo IOR file\n";
    $DCPSREPO->Kill ();
    exit 1;
}

$Publisher->Spawn ();
    
open (DATA, $data_file);

while ($line = <DATA>)
{
 if ($line =~ /Done writing/)
   {
    break;
   }
}

$Subscriber->Spawn ();

$PublisherResult = $Publisher->WaitKill (300);
if ($PublisherResult != 0) {
    print STDERR "ERROR: publisher returned $PublisherResult \n";
    $status = 1;
}

$SubscriberResult = $Subscriber->WaitKill (15);
if ($SubscriberResult != 0) {
    print STDERR "ERROR: subscriber returned $SubscriberResult \n";
    $status = 1;
}

$ir = $DCPSREPO->TerminateWaitKill(5);
if ($ir != 0) {
    print STDERR "ERROR: DCPSInfoRepo returned $ir\n";
    $status = 1;
}

unlink $dcpsrepo_ior;
unlink $data_file;

if ($status == 0) {
  print "test PASSED.\n";
} else {
  print STDERR "test FAILED.\n";
}

exit $status;
