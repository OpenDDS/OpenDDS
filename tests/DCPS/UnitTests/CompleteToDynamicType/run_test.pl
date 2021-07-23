eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# -*- perl -*-
use Sys::Hostname;

use Env (DDS_ROOT);
use lib "$DDS_ROOT/bin";
use Env (ACE_ROOT);
use lib "$ACE_ROOT/bin";
use PerlDDS::Run_Test;
use FileHandle;
use Cwd;
use strict;
my $status = 0;

my $test = new PerlDDS::TestFramework();
$test->process("ctdt", "CompleteToDynamicType", "");
$test->start_process("ctdt");
my $retcode = $test->finish(30);
if ($retcode != 0) {
  $status = 1;
}

exit $status;
