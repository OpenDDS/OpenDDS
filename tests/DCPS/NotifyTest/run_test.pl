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

my $opts = "-d -u";

if ($ARGV[0] eq 'unregister') {
    $opts = "-u";
}
elsif ($ARGV[0] eq 'dispose') {
    $opts = "-d";
}
elsif ($ARGV[0] ne '') {
    print STDERR "ERROR: invalid test case\n";
    exit 1;
}

my $test = new PerlDDS::TestFramework();

$test->process('pub', 'publisher', $opts);
$test->process('sub', 'subscriber', $opts);

$test->setup_discovery();
$test->start_process('pub');
$test->start_process('sub');

exit $test->finish(300);
