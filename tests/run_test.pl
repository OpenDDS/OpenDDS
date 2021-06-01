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

my $executable = $ARGV[0];

if ($executable eq '') {
    print STDERR "ERROR: no executable was specified\n";
    exit 1;
}

if (! -e $executable) {
    print STDERR "Executable $executable does not exist\n";
    # FUTURE: Return a status indicating that the test was not run.
    exit 0;
}

my $test = new PerlDDS::TestFramework();
$test->process($executable, $executable, "");
print STDERR "Running $executable\n";
$test->start_process($executable);
my $retcode = $test->finish(60);
if ($retcode != 0) {
    exit 1;
}

exit 0;
