package misc_utils;

use strict;
use warnings;

require Exporter;
our @ISA = qw(Exporter);
our @EXPORT_OK = qw(
  get_trace
  just_trace
  trace
);

sub get_trace {
  my $prefix = shift();
  my $offset = shift();
  $prefix = "$prefix: " if ($prefix);

  my $i = $offset;
  my $msg = $prefix . join('', @_) . "\n";
  while (my @call = (caller($i++))) {
    my @next = caller($i);
    my $from = @next ? $next[3] : 'main';
    $msg .= $prefix . "STACK TRACE[" . ($i - 1) . "] $call[1]:$call[2] in $from\n";
  }

  return $msg;
}

sub just_trace {
  print STDERR (get_trace('ERROR', 1, @_));
}

sub trace {
  die(get_trace('ERROR', 1, @_));
}

1;
