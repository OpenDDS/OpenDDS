eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# $Id$
# -*- perl -*-

use Env qw(DDS_ROOT ACE_ROOT);
use lib "$DDS_ROOT/bin";
use lib "$ACE_ROOT/bin";
use PerlDDS::Run_Test;

PerlDDS::add_lib_path('../FooType');

$pub_opts = "";
$sub_opts = "";

my $runTop ;# = 1;
# $pub_opts .= "-DCPSDebugLevel 1 -ORBVerboseLogging 0 ";
# $sub_opts .= "-DCPSDebugLevel 1 -ORBVerboseLogging 0 ";
# $pub_opts .= "-DCPSBit 0 ";
# $sub_opts .= "-DCPSBit 0 ";
# $pub_opts .= "-DCPSChunks 1 ";
my $norun;
# $norun = 1;

my $arg = shift;
if ($arg eq 'low') {
  $pub_opts .= "-t 8 -s 128";
  $sub_opts .= "-t 8 -n 1024";

} elsif ($arg eq 'medium') {
  $pub_opts .= "-t 16 -s 64";
  $sub_opts .= "-t 16 -n 1024";

} elsif ($arg eq 'high') {
  $pub_opts .= "-t 32 -s 32";
  $sub_opts .= "-t 32 -n 1024";

} elsif ($arg eq 'aggressive') {
  $pub_opts .= "-t 64 -s 16";
  $sub_opts .= "-t 64 -n 1024";

} elsif ($arg eq 'single') {
  $pub_opts .= "-t 1 -s 1";
  $sub_opts .= "-t 1 -n 1";

} else { # default (i.e. lazy)
  $pub_opts .= "-t 1 -s 1024";
  $sub_opts .= "-t 1 -n 1024";
}

$status = 0;

$dcpsrepo_ior = "repo.ior";

unlink $dcpsrepo_ior;

$DCPSREPO = PerlDDS::create_process("$ENV{DDS_ROOT}/bin/DCPSInfoRepo",
                                    "$-o $dcpsrepo_ior ");

$Subscriber = PerlDDS::create_process("subscriber", "$sub_opts");

$Publisher = PerlDDS::create_process("publisher", "$pub_opts ");

if( $norun) {
  print $DCPSREPO->CommandLine() . "\n";
  print $Subscriber->CommandLine() . "\n";
  print $Publisher->CommandLine() . "\n";
  exit;
}

print $DCPSREPO->CommandLine() . "\n";
$DCPSREPO->Spawn();
if (PerlACE::waitforfile_timed($dcpsrepo_ior, 30) == -1) {
    print STDERR "ERROR: waiting for DCPSInfo IOR file\n";
    $DCPSREPO->Kill ();
    exit 1;
}

print $Subscriber->CommandLine() . "\n";
$Subscriber->Spawn();

print $Publisher->CommandLine() . "\n";
$Publisher->Spawn();

my $topargs = "top -bd1 -p $Publisher->{PROCESS} >publisher-sizes.log &";
print "TOP COMMAND: " . $topargs . "\n";
system("$topargs") if $runTop;

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
