eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
     & eval 'exec perl -S $0 $argv:q'
     if 0;

# -*- perl -*-

use Env qw(ACE_ROOT DDS_ROOT);
use lib "$ACE_ROOT/bin";
use lib "$DDS_ROOT/bin";
use PerlDDS::Run_Test;
use PerlDDS::Process_Java;
use strict;

PerlACE::add_lib_path ('.');
my $CL = new PerlDDS::Process_Java ('TestUnion');
my $client_status = $CL->SpawnWaitKill (300);

if ($client_status != 0) {
    print STDERR "ERROR: client returned $client_status\n";
}

exit $client_status;
