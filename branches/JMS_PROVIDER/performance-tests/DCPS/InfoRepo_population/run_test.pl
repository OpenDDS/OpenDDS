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

$status = 0;

# my $debug = 1;
my $debug ;# = 1;
my $debuglevel;
my $orbdebuglevel;
my $verbose;
my $debugfile;
if( $debug) {
  $debuglevel = 10;
  $orbdebuglevel = 5;
  $verbose    = 1;
  $debugfile  = "log.out";
}

unlink $debugfile if $debugfile;

# my $intermittent = 1;
my $intermittent ;# = 1;
my $stackfile;
my $stackdumpcmd;
if( $intermittent) {
  $stackfile    = "stacks";
  $stackdumpcmd = "grabstacks";
}

$use_svc_conf = !new PerlACE::ConfigList->check_config ('STATIC');

$opts = $use_svc_conf ? " -ORBSvcConf ../../tcp.conf " : '';
$opts .= "-ORBDebugLevel $orbdebuglevel " if $orbdebuglevel;
$opts .= "-DCPSDebugLevel $debuglevel "   if $debuglevel;
$opts .= "-ORBVerboseLogging 1 "          if $verbose;
$opts .= "-ORBLogFile $debugfile "        if $debugfile;
$pub_opts = "$opts -DCPSConfigFile pub.ini -DCPSBit 0 -t5 -n5 -p5 -s5";
$sub_opts = "$opts -DCPSConfigFile sub.ini -DCPSBit 0 -t5 -n5 -s5 -p10";

my $syncopts  = "-ORBDebugLevel $orbdebuglevel " if $orbdebuglevel;
   $syncopts .= "-ORBVerboseLogging 1 "          if $verbose;
   $syncopts .= "-ORBLogFile $debugfile "        if $debugfile;

$domains_file = "domain_ids";
$dcpsrepo_ior = "repo.ior";
$sync_ior="sync.ior";
$repo_bit_opt = "$opts -NOBITS";

unlink $dcpsrepo_ior;

$DCPSREPO = PerlDDS::create_process ("$ENV{DDS_ROOT}/bin/DCPSInfoRepo",
				  "$repo_bit_opt -o $dcpsrepo_ior -d $domains_file");
$Subscriber = PerlDDS::create_process ("subscriber", " $sub_opts");
$Publisher = PerlDDS::create_process ("publisher", " $pub_opts");
$Publisher2 = PerlDDS::create_process ("publisher", " $pub_opts"." -i2");
$SyncServer = PerlDDS::create_process ("syncServer"
				    , "$syncopts -p2 -s1");

print $DCPSREPO->CommandLine() . "\n";
$DCPSREPO->Spawn ();
if (PerlACE::waitforfile_timed ($dcpsrepo_ior, 30) == -1) {
    print STDERR "ERROR: waiting for DCPSInfo IOR file\n";
    $DCPSREPO->Kill ();
    exit 1;
}

print $SyncServer->CommandLine() . "\n";
$SyncServer->Spawn ();
if (PerlACE::waitforfile_timed ($sync_ior, 20) == -1) {
    print STDERR "ERROR: waiting for Sync IOR file\n";
    $DCPSREPO->Kill ();
    $SyncServer->Kill ();
    exit 1;
}

print $Publisher->CommandLine() . "\n";
$Publisher->Spawn ();
print $Publisher2->CommandLine() . "\n";
$Publisher2->Spawn ();

print $Subscriber->CommandLine() . "\n";
$Subscriber->Spawn ();

if($intermittent) {
  $PublisherResult = $Publisher->TimedWait (300);
  if( 0 != $PublisherResult) {
    my $outfile = $stackfile . "_1";
    `$stackdumpcmd publisher subscriber syncServer DCPSInfoRepo > $outfile`;
  } else {
    print "Publisher completed without incident.\n";
  }
  $Publisher->Kill ();

} else {
  $PublisherResult = $Publisher->WaitKill (300);
}
if ($PublisherResult != 0) {
    print STDERR "ERROR: publisher returned $PublisherResult \n";
    $status = 1;
}

if($intermittent) {
  $PublisherResult2 = $Publisher2->TimedWait (300);
  if( 0 != $PublisherResult2) {
    my $outfile = $stackfile . "_2";
    `$stackdumpcmd publisher subscriber syncServer DCPSInfoRepo > $outfile`;
  } else {
    print "Publisher2 completed without incident.\n";
  }
  $Publisher2->Kill ();

} else {
  $PublisherResult2 = $Publisher2->WaitKill (300);
}
if ($PublisherResult2 != 0) {
    print STDERR "ERROR: publisher returned $PublisherResult2 \n";
    $status = 1;
}

if($intermittent) {
  $SubscriberResult = $Subscriber->TimedWait (15);
  if( 0 != $SubscriberResult) {
    my $outfile = $stackfile . "_3";
    `$stackdumpcmd subscriber syncServer DCPSInfoRepo > $outfile`;
  } else {
    print "Subscriber completed without incident.\n";
  }
  $Subscriber->Kill ();

} else {
  $SubscriberResult = $Subscriber->WaitKill (15);
}
if ($SubscriberResult != 0) {
    print STDERR "ERROR: subscriber returned $SubscriberResult \n";
    $status = 1;
}

if($intermittent) {
  $ss = $SyncServer->TimedWait (5);
  if( 0 != $ss) {
    my $outfile = $stackfile . "_4";
    `$stackdumpcmd syncServer DCPSInfoRepo > $outfile`;
  } else {
    print "SyncServer completed without incident.\n";
  }
  $SyncServer->Kill ();

} else {
  $ss = $SyncServer->WaitKill(5);
}
if ($ss != 0) {
    print STDERR "ERROR: SyncServer returned $ss\n";
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

