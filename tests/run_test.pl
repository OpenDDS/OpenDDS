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

my $single_test = $ARGV[0];

if ($single_test eq '') {
    print STDERR "ERROR: no test was specified\n";
    exit 1;
}

my $test = new PerlDDS::TestFramework();
$test->process("$single_test", "$single_test", "");
print STDERR "Running $single_test\n";
$test->start_process("$single_test");
my $retcode = $test->finish(60);
if ($retcode != 0) {
    exit 1;
}

exit 0;
