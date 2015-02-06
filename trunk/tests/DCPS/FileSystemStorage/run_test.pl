eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
     & eval 'exec perl -S $0 $argv:q'
     if 0;

# -*- perl -*-

use lib "$ENV{ACE_ROOT}/bin";
use lib "$ENV{DDS_ROOT}/bin";
use PerlDDS::Run_Test;
use File::Path;
use strict;

rmtree('test') if -d 'test';

my $TEST = PerlDDS::create_process('FileSystemStorageTest');
my $result = $TEST->SpawnWaitKill(60);
if ($result != 0) {
  print STDERR "ERROR: test returned $result\n";
  exit 1;
}

exit 0;
