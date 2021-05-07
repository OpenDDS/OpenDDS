eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# -*- perl -*-

use feature qw/say/;
use autodie;

my @command = splice @ARGV, 1;

#print "@command\n";

open (STDOUT, '>', $ARGV[0]);
open (STDERR, '>', $ARGV[0]);

exec { $ARGV[2] } @command;
