eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# -*- perl -*-

use Env qw(DDS_ROOT ACE_ROOT);
use lib "$DDS_ROOT/bin";
use lib "$ACE_ROOT/bin";
use PerlDDS::Run_Test;

PerlDDS::add_lib_path('../FooType');

$pub_opts = "";
$sub_opts = "";

# $pub_opts .= "-DCPSDebugLevel 10 -ORBVerboseLogging 0 ";
# $sub_opts .= "-DCPSDebugLevel 10 -ORBVerboseLogging 0 ";
# $pub_opts .= "-ORBLogFile pub.log ";
# $sub_opts .= "-ORBLogFile sub.log ";

# $pub_opts .= "-DCPSBit 0 ";
# $sub_opts .= "-DCPSBit 0 ";
# $pub_opts .= "-DCPSChunks 1 ";
my $norun;
# $norun = 1;


my $samples_per_pub = 8;
my $n_pubs = 1;
my $delay_between_pubs_msec = 500;
my $sub_work_sleep_msec = 100;
my $delay_between_cycles_msec = 250;
my $samples_per_cycle = 4;
my $use_cft = 1;
my $pub_deadline_msec = $delay_between_pubs_msec;
my $sub_deadline_msec = $pub_deadline_msec;

if ($#ARGV >= 0) {
  for (my $i = 0; $i <= $#ARGV; $i++) {
    if ($ARGV[$i] eq '-samples_per_cycle') {
      $i++;
      $samples_per_cycle = $ARGV[$i];
    }
    elsif ($ARGV[$i] eq '-n_samples_per_pub') {
      $i++;
      $n_samples_per_pub = $ARGV[$i];
    }
    elsif ($ARGV[$i] eq '-n_pubs') {
      $i++;
      $n_pubs = $ARGV[$i];
    }
    elsif ($ARGV[$i] eq '-delay_between_pubs') {
      $i++;
      $delay_between_pubs_msec = $ARGV[$i];
    }
    elsif ($ARGV[$i] eq '-sub_work_sleep') {
      $i++;
      $sub_work_sleep_msec = $ARGV[$i];
    }
    elsif ($ARGV[$i] eq '-delay_between_cycles') {
      $i++;
      $delay_between_cycles_msec = $ARGV[$i];
    }
    elsif ($ARGV[$i] eq '-pub_deadline') {
      $i++;
      $pub_deadline_msec = $ARGV[$i];
    }
    elsif ($ARGV[$i] eq '-sub_deadline') {
      $i++;
      $sub_deadline_msec = $ARGV[$i];
    }
    elsif ($ARGV[$i] eq '-use_cft') {
      $i++;
      $use_cft = $ARGV[$i];
    }
  }
}

my $n_pub_samples = $samples_per_pub * $n_pubs;

$pub_opts .= "-t $n_pubs -s $samples_per_pub -d $delay_between_pubs_msec ".
    "-l $pub_deadline_msec";

$sub_opts .= "-t $n_pubs -n $n_pub_samples -c $samples_per_cycle ".
    "-s $sub_work_sleep_msec -d $delay_between_cycles_msec ".
    "-l $sub_deadline_msec -f $use_cft";

$status = 0;

$dcpsrepo_ior = "repo.ior";

unlink $dcpsrepo_ior;

$DCPSREPO = PerlDDS::create_process("$ENV{DDS_ROOT}/bin/DCPSInfoRepo",
                                    "-o $dcpsrepo_ior");

$Subscriber = PerlDDS::create_process("subscriber", "$sub_opts");

$Publisher = PerlDDS::create_process("publisher", "$pub_opts");

if( $norun) {
  print $DCPSREPO->CommandLine() . "\n";
  print $Subscriber->CommandLine() . "\n";
  print $Publisher->CommandLine() . "\n";
  exit;
}

print $DCPSREPO->CommandLine() . "\n";
$DCPSREPO->Spawn();
if (PerlACE::waitforfile_timed($dcpsrepo_ior, 30) == -1) {
    print STDERR "ERROR: waiting for Info Repo IOR file\n";
    $DCPSREPO->Kill ();
    exit 1;
}

print $Publisher->CommandLine() . "\n";
$Publisher->Spawn();

sleep(2);

print $Subscriber->CommandLine() . "\n";
$Subscriber->Spawn();

$SubscriberResult = $Subscriber->WaitKill(300);
if ($SubscriberResult != 0) {
  print STDERR "ERROR: subscriber returned $SubscriberResult \n";
  $status = 1;
}

$PublisherResult = $Publisher->WaitKill (15);
if ($PublisherResult != 0) {
  print STDERR "ERROR: publisher returned $PublisherResult \n";
  $status = 1;
}

$ir = $DCPSREPO->TerminateWaitKill(5);
if ($ir != 0) {
  print STDERR "ERROR: DCPSInfoRepo returned $ir\n";
  $status = 1;
}

unlink $dcpsrepo_ior;

if ($status == 0) {
  print "test PASSED.\n";
} else {
  print STDERR "test FAILED.\n";
}

exit $status;
