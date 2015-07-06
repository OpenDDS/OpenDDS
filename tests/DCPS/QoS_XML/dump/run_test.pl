eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
     & eval 'exec perl -S $0 $argv:q'
     if 0;

# -*- perl -*-

use lib "$ENV{ACE_ROOT}/bin";
use PerlACE::TestTarget;

my $program = PerlACE::TestTarget::create_target (1) || die "Create target 1 failed\n";

$PROG = $program->CreateProcess ("qos_dump");
$program_status = $PROG->SpawnWaitKill ($program->ProcessStartWaitInterval ());

if ($program_status != 0) {
    print STDERR "ERROR: qos_dump returned $program_status\n";
    exit 1;
}

exit 0;
