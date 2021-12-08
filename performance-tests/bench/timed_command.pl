eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# -*- perl -*-

use feature qw/say/;
use autodie;

use IPC::Cmd qw[can_run run run_forked];

my @command = splice @ARGV, 1;

print "@command\n";

my $result = run_forked( "@command", { 'timeout' => "$ARGV[0]" });

exit 0;
