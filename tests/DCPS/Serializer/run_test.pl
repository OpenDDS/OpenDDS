eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
     & eval 'exec perl -S $0 $argv:q'
     if 0;

# -*- perl -*-

use Env (DDS_ROOT);
use lib "$DDS_ROOT/bin";
use Env (ACE_ROOT);
use lib "$ACE_ROOT/bin";
use PerlDDS::Run_Test;

my $args = "--test" ;

my $test = new PerlDDS::TestFramework();
$test->process("SerializerTest", "SerializerTest", $args);
$test->start_process("SerializerTest");
my $status = $test->finish(60);
print STDERR "ERROR: client returned $status\n" if $status ;
exit ($status ne 0)? 1 : 0 ;
