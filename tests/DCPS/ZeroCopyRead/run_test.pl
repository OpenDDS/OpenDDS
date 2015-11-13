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

my $status = 0;

# -b
my $parameters = "-DcpsBit 0";
# or could have
# $parameters = "-b -DcpsBit 1";

if ($ARGV[0] eq 'by_instance') {
  $parameters .= " -i";
}

my $test = new PerlDDS::TestFramework();

$test->setup_discovery("-NOBITS") if !$PerlDDS::SafetyProfile;
$test->process("main", "main", $parameters);
$test->start_process("main");
my $result = $test->finish(60);
if ($result != 0) {
    print STDERR "ERROR: main returned $result \n";
    $status = 1;
}

exit $status;
