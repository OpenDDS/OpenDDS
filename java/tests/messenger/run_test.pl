#!/usr/bin/env perl

eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
     & eval 'exec perl -S $0 $argv:q'
     if 0;

# -*- perl -*-

use Env qw(ACE_ROOT DDS_ROOT);
use lib "$DDS_ROOT/bin";
use lib "$ACE_ROOT/bin";
use PerlDDS::Run_Test;
use PerlDDS::Process_Java;
use strict;
my $status = 0;
my $debug = '0';

foreach my $i (@ARGV) {
    if ($i eq '-debug') {
        $debug = '10';
    }
}

my $config = pop;
if ($config eq '') {
    $config = 'tcp';
}

my $use_repo = ($config !~ /^rtps_disc/);

my $reliable = '-r';
my $wait_for_acks = '-w';

if ($config eq 'udp') {
  $reliable = '';
}

if (($config =~ 'rtps') || ($config =~ 'rtps_uni') ||
    ($config eq 'rtps_disc') || ($config eq 'udp')) {
  $wait_for_acks = '';
}

my $opts = "-DCPSBit 0 -DCPSConfigFile $config.ini $reliable $wait_for_acks";
my $pub_opts = $opts;
my $sub_opts = $opts;
if ($debug ne '0') {
    my $debug_opt = "-ORBDebugLevel $debug -DCPSDebugLevel $debug " .
                    "-DCPSTransportDebugLevel $debug";
    $pub_opts .= " $debug_opt -ORBLogFile pub.log";
    $sub_opts .= " $debug_opt -ORBLogFile sub.log";
}

my $dcpsrepo_ior = 'repo.ior';

unlink $dcpsrepo_ior;
unlink qw/pub.log sub.log DCPSInfoRepo.log/;

my $DCPSREPO = PerlDDS::create_process("$DDS_ROOT/bin/DCPSInfoRepo",
                 '-NOBITS' . (($debug eq '0' ? '' : " -ORBDebugLevel $debug" .
                 ' -ORBLogFile DCPSInfoRepo.log')) . " -o $dcpsrepo_ior");

my $SUB = new PerlDDS::Process_Java('TestSubscriber', $sub_opts,
            ["$DDS_ROOT/java/tests/messenger/messenger_idl/".
             'messenger_idl_test.jar', 'subscriber/classes']);

PerlACE::add_lib_path("$DDS_ROOT/java/tests/messenger/messenger_idl");

my $PUB = new PerlDDS::Process_Java('TestPublisher', $pub_opts,
            ["$DDS_ROOT/java/tests/messenger/messenger_idl/".
             'messenger_idl_test.jar', 'publisher/classes']);

if ($use_repo) {
    print $DCPSREPO->CommandLine() . "\n";
    $DCPSREPO->Spawn();
    if (PerlACE::waitforfile_timed($dcpsrepo_ior, 30) == -1) {
        print STDERR "ERROR: waiting for DCPSInfo IOR file\n";
        $DCPSREPO->Kill();
        exit 1;
    }
}

print $PUB->CommandLine() . "\n";
$PUB->Spawn();

print $SUB->CommandLine() . "\n";
$SUB->Spawn();

my $PublisherResult = $PUB->WaitKill(300);
if ($PublisherResult != 0) {
    print STDERR "ERROR: publisher returned $PublisherResult\n";
    $status = 1;
}

my $SubscriberResult = $SUB->WaitKill(30);
if ($SubscriberResult != 0) {
    print STDERR "ERROR: subscriber returned $SubscriberResult\n";
    $status = 1;
}

if ($use_repo) {
    my $ir = $DCPSREPO->TerminateWaitKill(5);
    if ($ir != 0) {
        print STDERR "ERROR: DCPSInfoRepo returned $ir\n";
        $status = 1;
    }
    unlink $dcpsrepo_ior;
}

if ($status == 0) {
    print "test PASSED.\n";
} else {
    print STDERR "test FAILED.\n";
}

exit $status;
