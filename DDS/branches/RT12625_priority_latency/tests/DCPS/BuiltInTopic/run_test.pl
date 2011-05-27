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

PerlDDS::add_lib_path('../FooType4');

$ignore_kind=0;

if ($ARGV[0] eq 'ignore_part') {
  $ignore_kind = 1;
}
elsif ($ARGV[0] eq 'ignore_topic') {
  $ignore_kind = 2;
}
elsif ($ARGV[0] eq 'ignore_pub') {
  $ignore_kind = 3;
}
elsif ($ARGV[0] eq 'ignore_sub') {
  $ignore_kind = 4;
}
elsif ($ARGV[0] eq '') {
  # default get_builtin_subscriber test
  $ignore_kind = 0;
}
else {
  print STDERR "ERROR: invalid parameter $ARGV[0] \n";
  exit 1;
}

$iorfile = "repo.ior";
unlink $iorfile;
$status = 0;
$client_orb = "";

my $repoDebug;
my $appDebug;
# $repoDebug = 10;
# $appDebug  = 10;

my $repoOpts = "";
$repoOpts  = "-DCPSDebugLevel $repoDebug "               if $repoDebug;
$repoOpts .= "-DCPSTransportDebugLevel $transportDebug " if $transportDebug;

my $appOpts = "";
$appOpts  = "-DCPSDebugLevel $appDebug "                if $appDebug;
$appOpts .= "-DCPSTransportDebugLevel $transportDebug " if $transportDebug;

$dynamic_tcp = new PerlACE::ConfigList->check_config ('STATIC')
    ? '' : '-ORBSvcConf ../../tcp.conf';

$REPO = PerlDDS::create_process ("$ENV{DDS_ROOT}/bin/DCPSInfoRepo",
                                "$repoOpts -o $iorfile $dynamic_tcp ");
$CL = PerlDDS::create_process ("bit", "-DCPSInfoRepo file://$iorfile " .
                              "$dynamic_tcp -i $ignore_kind $appOpts ");

print $REPO->CommandLine() . "\n";
$REPO->Spawn ();

if (PerlACE::waitforfile_timed ($iorfile, 30) == -1) {
    print STDERR "ERROR: cannot find file <$iorfile>\n";
    $REPO->Kill (); $REPO->TimedWait (1);
    exit 1;
}

print $CL->CommandLine() . "\n";
$result = $CL->SpawnWaitKill (60);

if ($result != 0) {
    print STDERR "ERROR: client returned $result\n";
    $status = 1;
}

$REPO->TerminateWaitKill (5);

unlink $iorfile;

if ($status == 0) {
  print "test PASSED.\n";
}
else {
  print STDERR "test FAILED.\n";
}
exit $status;
