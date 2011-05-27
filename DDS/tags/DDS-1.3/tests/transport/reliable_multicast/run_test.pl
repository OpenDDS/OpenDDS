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

#
# Create the test object.
#
$test = PerlDDS::create_process("unittest") ;

if ($test->SpawnWaitKill(600) == -1) {
    print STDERR "ERROR: unittest did not finish.\n";
    exit 1;
}

exit 0 ;
