eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
     & eval 'exec perl -S $0 $argv:q'
     if 0;

# -*- perl -*-

use Env qw(ACE_ROOT JAVA_HOME DDS_ROOT TAO_ROOT);
use lib "$DDS_ROOT/bin";
use lib "$ACE_ROOT/bin";
use DDS_Run_Test;
use strict;

my $status = 0;
my $debug = '0';

foreach my $i (@ARGV) {
    if ($i eq '-debug') {
        $debug = '10';
    } 
}

my $transport = pop;
if ($transport eq "") {
    print "ERROR: transport not specified!\n";
    exit 1;
}

my $opts = "-ORBSvcConf $DDS_ROOT/DevGuideExamples/DCPS/Messenger/$transport.conf";
my $debug_opt = ($debug eq '0') ? ''
    : "-ORBDebugLevel $debug -DCPSDebugLevel $debug";
my $pub_opts = "$opts -ORBListenEndpoints iiop://127.0.0.1:12346 $debug_opt ".
    "-ORBLogFile pub.log -DCPSConfigFile pub_$transport.ini";
my $sub_opts = "$opts -ORBListenEndpoints iiop://127.0.0.1:12347 $debug_opt ".
    "-ORBLogFile sub.log -DCPSConfigFile sub_$transport.ini";

my $domains_file = "domain_ids";
my $dcpsrepo_ior = "repo.ior";

unlink $dcpsrepo_ior;

my $DCPSREPO = PerlDDS::create_process ("$DDS_ROOT/bin/DCPSInfoRepo",
               "-ORBSvcConf $DDS_ROOT/DevGuideExamples/DCPS/Messenger/tcp.conf ".
               "-ORBListenEndpoints iiop://127.0.0.1:1111 -ORBDebugLevel 10 ".
               "-ORBLogFile DCPSInfoRepo.log -o $dcpsrepo_ior ".
               "-d $domains_file");

my $SUB = PerlDDS::create_process ("$DDS_ROOT/DevGuideExamples/DCPS/Messenger".
                                   "/subscriber", "$sub_opts");

PerlACE::add_lib_path ("$DDS_ROOT/lib");
PerlACE::add_lib_path ("$DDS_ROOT/java/tests/messenger/messenger_idl");
my @classpaths = ('classes', "$DDS_ROOT/lib/i2jrt.jar",
                  "$DDS_ROOT/lib/OpenDDS_DCPS.jar",
                  "$DDS_ROOT/java/tests/messenger/messenger_idl/".
                      "messenger_idl_test.jar");
my $sep = ':';
my $jnid;
if ($^O eq 'MSWin32') {
    $sep = ';';
    $jnid = '-Djni.nativeDebug=1'
        unless $PerlACE::Process::ExeSubDir =~ /Release/i;
}
my $classpath = join ($sep, @classpaths);
my $PUB = PerlDDS::create_process ("$JAVA_HOME/bin/java",
                                   "-Xcheck:jni -cp $classpath $jnid " .
                                   "TestPublisher $pub_opts");
$PUB->IgnoreExeSubDir (1);

$DCPSREPO->Spawn ();
if (PerlACE::waitforfile_timed ($dcpsrepo_ior, 30) == -1) {
    print STDERR "ERROR: waiting for DCPSInfo IOR file\n";
    $DCPSREPO->Kill ();
    exit 1;
}

$PUB->Spawn ();

$SUB->Spawn ();

my $PublisherResult = $PUB->WaitKill (300);
if ($PublisherResult != 0) {
    print STDERR "ERROR: publisher returned $PublisherResult \n";
    $status = 1;
}

my $SubscriberResult = $SUB->WaitKill (15);
if ($SubscriberResult != 0) {
    print STDERR "ERROR: subscriber returned $SubscriberResult \n";
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
