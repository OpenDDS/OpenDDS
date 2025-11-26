package misc_utils;

use strict;
use warnings;

require Exporter;
our @ISA = qw(Exporter);
our @EXPORT_OK = qw(
  get_trace
  just_trace
  trace
  parse_func_opts
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

sub parse_func_opts {
  my $valid_args = shift();
  my @check_valid_args = %{$valid_args};
  if (scalar(@check_valid_args) % 2 != 0) {
    trace("valid_args is not a valid hash, check parse_func_opts");
  }
  if (scalar(@_) % 2 != 0) {
    my $list = join(' ', @_);
    trace("optional args passed ($list) are not a valid hash, check function call");
  }
  my %args = (%{$valid_args}, @_);
  my @invalid_args = grep { !exists($valid_args->{$_}) } keys(%args);
  trace("invalid arguments: ", join(', ', @invalid_args)) if (scalar(@invalid_args));
  return \%args;
}

1;
