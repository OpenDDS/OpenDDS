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

my $args = "--test" ;

$test = PerlDDS::create_process( "./SerializerTest", $args) ;

my $status = $test->SpawnWaitKill( 60);
print STDERR "ERROR: client returned $status\n" if $status ;

exit ($status ne 0)? 1 : 0 ;

