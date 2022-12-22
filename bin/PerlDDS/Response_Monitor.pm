package Response_Monitor;

use strict;
use Time::HiRes;

sub new {
  # $delay = seconds between measurement
  # $drift = allowed msecs of clock drift
  my ($self, $delay, $drift) = @_;

  # defaults
  $delay = 1 if !defined $delay;
  $drift = 500 if !defined $drift;

  if (my $pid = fork) {
    # parent
    bless { delay => $delay,
            drift => $drift,
            child => $pid }, $self;
  } else {
    print "Response monitor running with $delay second delay and $drift msecs of drift\n";

    # child
    my $last;
    my $now;
    my $diff_ms;

    while (1) {
      $last = Time::HiRes::gettimeofday();
      Time::HiRes::sleep $delay;
      $now = Time::HiRes::gettimeofday();
      $diff_ms = 1000 * ($now - $last - $delay);

      if ($diff_ms > $drift) {
        print "WARNING: Response monitor reports $diff_ms milliseconds drift.\n";
      }
    }
  }
}

sub stop {
  my $self = shift;
  kill 1, $self->{child};
  print "Response monitor pid " . $self->{child} . " stopped.\n";
  $self->{child} = -1;
}

sub DESTROY {
  my $self = shift;
  $self->stop() if $self->{child} != -1;
}

1;
