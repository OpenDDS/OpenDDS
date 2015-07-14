eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# -*- perl -*-

use Env (DDS_ROOT);
use lib "$DDS_ROOT/bin";
use Env (ACE_ROOT);
use lib "$ACE_ROOT/bin";
use PerlDDS::Run_Test;

$status = 0;

PerlDDS::add_lib_path('../FooType4');
PerlDDS::add_lib_path('../common');

# single reader with single instances test
$multiple_instance=0;
$num_samples_per_reader=10;
$num_readers=1;
$use_take=0;

# multiple instances test
if ($ARGV[0] eq 'mi') {
  $multiple_instance=1;
  $num_samples_per_reader=10;
  $num_readers=1;
}
# multiple datareaders with single instance test
elsif ($ARGV[0] eq 'mr') {
  $multiple_instance=0;
  $num_samples_per_reader=5;
  $num_readers=2;
}
# multiple datareaders with multiple instances test
elsif ($ARGV[0] eq 'mri') {
  $multiple_instance=1;
  $num_samples_per_reader=4;
  $num_readers=3;
}
# multiple datareaders with multiple instances test
elsif ($ARGV[0] eq 'mrit') {
  $multiple_instance=1;
  $num_samples_per_reader=4;
  $num_readers=3;
  $use_take=1;
}
elsif ($ARGV[0] eq '') {
  #default test - single datareader single instance.
}
else {
  print STDERR "ERROR: invalid parameter $ARGV[0] \n";
  exit 1;
}

$dcpsrepo_ior = "repo.ior";


unlink $dcpsrepo_ior;
unlink $pub_id_file;

# test multiple cases
$parameters = "-r $num_readers -t $use_take"
              . " -m $multiple_instance -i $num_samples_per_reader " ;


$DCPSREPO = PerlDDS::create_process ("$ENV{DDS_ROOT}/bin/DCPSInfoRepo",
                                    "-o $dcpsrepo_ior ");

$FooTest4 = PerlDDS::create_process ("FooTest4", $parameters);

print $FooTest4->CommandLine(), "\n";


$DCPSREPO->Spawn ();

if (PerlACE::waitforfile_timed ($dcpsrepo_ior, 30) == -1) {
    print STDERR "ERROR: waiting for Info Repo IOR file\n";
    $DCPSREPO->Kill ();
    exit 1;
}


$FooTest4->Spawn ();

$result = $FooTest4->WaitKill (60);

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
