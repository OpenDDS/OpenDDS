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

my $TESTDVR = PerlDDS::create_process("value_annotation");

my $status = $TESTDVR->SpawnWaitKill(300);

if ($status != 0) {
  print STDERR "ERROR: value_annotation returned $status\n";
  $status = 1;
}

exit $status;
