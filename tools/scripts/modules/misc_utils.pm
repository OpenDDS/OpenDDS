package misc_utils;

use strict;
use warnings;

require Exporter;
our @ISA = qw(Exporter);
our @EXPORT_OK = qw(
  trace
);

sub trace {
  my $i = 0;
  my $msg = "ERROR: " . join('', @_) . "\n";
  while (my @call = (caller($i++))) {
    my @next = caller($i);
    my $from = @next ? $next[3] : 'main';
    $msg .= "ERROR: STACK TRACE[" . ($i - 1) . "] $call[1]:$call[2] in $from\n";
  }
  die($msg);
}

1;
