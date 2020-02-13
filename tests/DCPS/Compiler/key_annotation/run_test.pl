eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
     & eval 'exec perl -S $0 $argv:q'
     if 0;

use Env (DDS_ROOT);
use lib "$DDS_ROOT/bin";
use Env (ACE_ROOT);
use lib "$ACE_ROOT/bin";
use PerlDDS::Run_Test;
use strict;

my $name = 'key_annotation';

my $status = 0;
my $TESTDVR = PerlDDS::create_process ($name);

my $status = $TESTDVR->SpawnWaitKill (300);
if ($status != 0) {
    print STDERR "ERROR: $name returned $status\n";
    $status = 1;
}

exit $status;
