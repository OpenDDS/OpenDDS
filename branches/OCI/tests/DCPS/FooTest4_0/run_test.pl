eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# $Id$
# -*- perl -*-

use lib '../../../../../bin';
use PerlACE::Run_Test;

$status = 0;

PerlACE::add_lib_path('../FooType4');
PerlACE::add_lib_path('../common');

if ($ARGV[0] eq '') { 
  #default test - single datareader single instance.
}
else {
  print STDERR "ERROR: invalid parameter $ARGV[0] \n";
  exit 1;
}

$domains_file = PerlACE::LocalFile ("domain_ids");
$dcpsrepo_ior = PerlACE::LocalFile ("repo.ior");

unlink $dcpsrepo_ior; 
unlink $pub_id_file;

$DCPSREPO = new PerlACE::Process ("../../../dds/InfoRepo/DCPSInfoRepo",
                             "-o $dcpsrepo_ior"
                             . " -d $domains_file -ORBDebugLevel 1");



# test multiple cases
$parameters = " -z " ;
$FooTest4 = new PerlACE::Process ("FooTest4_0", $parameters);
print $FooTest4->CommandLine(), "\n";


$DCPSREPO->Spawn ();

if (PerlACE::waitforfile_timed ($dcpsrepo_ior, 30) == -1) {
    print STDERR "ERROR: waiting for DCPSInfo IOR file\n";
    $DCPSREPO->Kill ();
    exit 1;
}


$FooTest4->Spawn ();

$result = $FooTest4->WaitKill (120);

if ($result != 0) {
    print STDERR "ERROR: FooTest4 returned $result \n";
    $status = 1;
}


$ir = $DCPSREPO->TerminateWaitKill(5);

if ($ir != 0) {
    print STDERR "ERROR: DCPSInfoRepo returned $ir\n";
    $status = 1;
}

exit $status;
