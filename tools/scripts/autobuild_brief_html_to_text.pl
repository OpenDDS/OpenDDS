#!/usr/bin/perl

use strict; use warnings;

use HTML::Parser;
use Term::ANSIColor;

use HTML::Parser 3.00 ();

my %inside;
my $needs_groupend = 0;
my $gh_actions = ($ENV{GITHUB_ACTIONS} // "") eq "true";

sub tag {
  my($tag, $num) = @_;
  $inside{$tag} += $num;
  if ($tag =~ /h[1-3]/) {
    if ($num > 0 && $needs_groupend) {
      $needs_groupend = 0;
      print "::endgroup::\n";
    }
    print "\n";  # not for all tags
  }
  if ($tag eq "br") {
    print "\n";  # not for all tags
  }
  if ($tag eq "body" && $num < 0) {
    if ($num < 0 && $needs_groupend) {
      $needs_groupend = 0;
      print "::endgroup::\n";
    }
    print "\n";  # not for all tags
  }
}

sub text {
  return if $inside{script} || $inside{style};
  my $esc = 1;
  my $line = $_[0];
  $line =~ s/\R//g;
  if ($inside{h2}) {
    print color 'yellow';
  }
  elsif ($inside{h3}) {
    if ($gh_actions && !$needs_groupend) {
      $needs_groupend = 1;
      print "::group::";
    }
    print color 'red';
  }
  else {
    $esc = 0;
  }
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
