eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# $Id$
# -*- perl -*-

use strict;
use warnings;

use Env qw(DDS_ROOT ACE_ROOT);
use lib "$DDS_ROOT/bin";
use lib "$ACE_ROOT/bin";
use PerlDDS::Run_Test;

PerlDDS::add_lib_path('../FooType');

my $runTop ;# = 1;
my $norun;
# $norun = 1;

my $status = 0;
my $nobit;
my $dlevel = 0;
my $dfile  = "test.log";

my $debug_opts = "";
$debug_opts .= "-DCPSDebugLevel $dlevel -ORBVerboseLogging 1 " if $dlevel;
$debug_opts .= "-ORBLogFile $dfile " if $dlevel and $dfile;

my $bit_opts = "";
$bit_opts = "DCPSBit 0 " if $nobit;

my $repo_bit_opts = "";
$repo_bit_opts = "-NOBITS " if $nobit;

my $pub_opts = "";
my $sub_opts = "";
$pub_opts .= "$debug_opts $bit_opts";
$sub_opts .= "$debug_opts $bit_opts";
# $pub_opts .= "-DCPSChunks 1 ";

my $arg = shift || "";
if ("$arg" eq 'low') {
  $pub_opts .= "-t 8 -s 128";
  $sub_opts .= "-t 8 -n 1024";

} elsif ("$arg" eq 'medium') {
  $pub_opts .= "-t 16 -s 64";
  $sub_opts .= "-t 16 -n 1024";

} elsif ("$arg" eq 'high') {
  $pub_opts .= "-t 32 -s 32";
  $sub_opts .= "-t 32 -n 1024";

} elsif ("$arg" eq 'aggressive') {
  $pub_opts .= "-t 64 -s 16";
  $sub_opts .= "-t 64 -n 1024";

} elsif ("$arg" eq 'single') {
  $pub_opts .= "-t 1 -s 1";
  $sub_opts .= "-t 1 -n 1";

} else { # default (i.e. lazy)
  $pub_opts .= "-t 1 -s 1024";
  $sub_opts .= "-t 1 -n 1024";
}

my $dcpsrepo_ior = "repo.ior";

unlink $dfile if $dfile;
unlink $dcpsrepo_ior;

my $DCPSREPO = PerlDDS::create_process("$ENV{DDS_ROOT}/bin/DCPSInfoRepo",
                                       "$repo_bit_opts -o $dcpsrepo_ior");

my $Subscriber = PerlDDS::create_process("subscriber", "$sub_opts");

my $Publisher = PerlDDS::create_process("publisher", "$pub_opts ");

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

print $Subscriber->CommandLine() . "\n";
$Subscriber->Spawn();

print $Publisher->CommandLine() . "\n";
$Publisher->Spawn();

my $topargs = "top -bd1 -p $Publisher->{PROCESS} >publisher-sizes.log &";
print "TOP COMMAND: " . $topargs . "\n";
system("$topargs") if $runTop;

my $SubscriberResult = $Subscriber->WaitKill(300);
if ($SubscriberResult != 0) {
  print STDERR "ERROR: subscriber returned $SubscriberResult \n";
  $status = 1;
}

my $PublisherResult = $Publisher->WaitKill (15);
if ($PublisherResult != 0) {
  print STDERR "ERROR: publisher returned $PublisherResult \n";
  $status = 1;
}

my $ir = $DCPSREPO->TerminateWaitKill(5);
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
