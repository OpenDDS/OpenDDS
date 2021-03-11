eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# -*- perl -*-

use Env (DDS_ROOT);
use lib "$DDS_ROOT/bin";
use Env (ACE_ROOT);
use lib "$ACE_ROOT/bin";
use PerlDDS::Run_Test;
use strict;

my $status = 0;
my $common_opts = "-ORBDebugLevel 10 -DCPSDebugLevel 10";

my $pub_opts = "$common_opts -ORBLogFile publisher.log";
my $sub_opts = "$common_opts -DCPSTransportDebugLevel 6 " .
               "-ORBLogFile subscriber.log";

my $Subscriber = PerlDDS::create_process ("subscriber", " $sub_opts");
my $Publisher = PerlDDS::create_process ("publisher", " $pub_opts");

print $Publisher->CommandLine() . "\n";
$Publisher->Spawn ();

print $Subscriber->CommandLine() . "\n";
$Subscriber->Spawn ();

my $PublisherResult = $Publisher->WaitKill (300);
if ($PublisherResult != 0) {
    print STDERR "ERROR: publisher returned $PublisherResult\n";
    $status = 1;
}

my $SubscriberResult = $Subscriber->WaitKill (15);
if ($SubscriberResult != 0) {
    print STDERR "ERROR: subscriber returned $SubscriberResult\n";
    $status = 1;
}

exit $status;
