#!/usr/bin/perl

use strict; use warnings;

use HTML::Parser;
use Term::ANSIColor;

use HTML::Parser 3.00 ();

my %inside;

sub tag {
  my($tag, $num) = @_;
  $inside{$tag} += $num;
  if ($tag =~ /h[1-3]/) {
    print "\n";  # not for all tags
  }
  if ($tag eq "br") {
    print "\n";  # not for all tags
  }
  if ($tag eq "body" && $num < 0) {
    print "\n";  # not for all tags
  }
}

sub text {
  return if $inside{script} || $inside{style};
  my $esc = 1;
  if ( $inside{h2}) {
    print color 'yellow';
  }
  elsif ( $inside{h3}) {
    print color 'red';
  }
  else {
    $esc = 0;
  }
  my $line = $_[0];
  $line =~ s/\R//g;
  print $line;
  print color 'reset' if $esc;
}

HTML::Parser->new(api_version => 3,
  handlers => [
    start => [\&tag, "tagname, '+1'"],
    end   => [\&tag, "tagname, '-1'"],
    text  => [\&text, "dtext"],
  ],
  marked_sections => 1,
)->parse_file(shift) || die "Can't open file: $!\n";;
