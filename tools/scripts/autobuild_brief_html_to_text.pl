#!/usr/bin/perl

use strict; use warnings;

use HTML::Parser;
use Term::ANSIColor;

use HTML::Parser 3.00 ();

my %inside;
#my $output = "";
my %output;
my $line_count = 0;

sub tag {
  my($tag, $num) = @_;
  $inside{$tag} += $num;
  if ($tag =~ /h[1-3]/) {
    print "\n";  # not for all tags
    #$output = $output . "\n";
    $output{$line_count++} = {"color" => "none", "text" => "\n"};
  }
  if ($tag eq "br") {
    print "\n";  # not for all tags
    #$output = $output . "\n";
    $output{$line_count++} = {"color" => "none", "text" => "\n"};
  }
  if ($tag eq "body" && $num < 0) {
    print "\n";  # not for all tags
    #$output = $output . "\n";
    $output{$line_count++} = {"color" => "none", "text" => "\n"};
  }
}

sub text {
  return if $inside{script} || $inside{style};
  my $esc = 1;
  my $color;
  if ( $inside{h2}) {
    print color 'yellow';
    $color = 'yellow';
    #$output{$line_count} = {"color" => "yellow"};
  }
  elsif ( $inside{h3}) {
    print color 'red';
    $color = 'red';
    #$output{$line_count} = {"color" => "red"};
  }
  else {
    $color = "none";
    $esc = 0;
  }
  my $line = $_[0];
  $line =~ s/\R//g;
  print $line;
  #$output = $output . $line;
  if ($line) {
    $output{$line_count++} = {"color" => $color, "text" => $line};
  }
  print color 'reset' if $esc;
}

sub parse_html {
  my $html_file = shift;
  HTML::Parser->new(api_version => 3,
    handlers => [
      start => [\&tag, "tagname, '+1'"],
      end   => [\&tag, "tagname, '-1'"],
      text  => [\&text, "dtext"],
    ],
    marked_sections => 1,
  )->parse_file($html_file) || die "Can't open file: $!\n";
}

my %traces;

# Collect stack traces from autobuild's output.log
sub collect_stack_trace {
  my $logfile = shift;
  open(my $fh, $logfile) or die "Couldn't open file $logfile: $!\n";

  my $test_prefix = "auto_run_tests:";
  my $trace_begin = "Begin stack trace";
  my $trace_end = "End stack trace";

  my $line = <$fh>;
  while (defined($line)) {
    my $idx = index($line, $test_prefix);
    if ($idx != -1) {
      my $test_name = substr($line, 16);

      while (defined($line = <$fh>) && index($line, $test_prefix) == -1) {
        if (index($line, $trace_begin) != -1) {
          my $trace = "";
          do {
            $trace = $trace . $line;
            $line = <$fh>;
          } while (index($line, $trace_end) == -1);
          $trace = $trace . $line;
          $traces{$test_name} = $trace;
        }
      }
    } else {
      $line = <$fh>;
    }
  }
#  foreach my $test (keys %traces) {
#    print color("red"), "$test", color("reset");
#    print "$traces{$test}\n";
#  }
  close($fh);
}

parse_html(shift);

print "CONTENTS OF OUTPUT:\n";
foreach my $k (sort {$a <=> $b} keys %output) {
  print "Item $k: $output{$k}->{text}";
}

collect_stack_trace(shift);

sub print_out {
  my $text = shift;
  my $color = shift;
  if ($color eq "none") {
    print $text;
  } else {
    print color($color), $text, color("reset");
  }
}

sub merge_output {
  my $test;
  foreach my $i (sort {$a <=> $b} keys %output) {
    my $color = $output{$i}->{"color"};
    my $text = $output{$i}->{"text"};
    if ($text eq "\n") {
      print "---- Blank line";
    }
    #if (defined $test && exists $traces{$test . "\n"}) {
    #  print "There is trace for test $test\n";
    #}
    my $tmp = $i + 1;
    if ($color eq "red") {
      $test = $text;
      print_out($text, "red");
    } elsif (defined $test && exists $traces{$test . "\n"} &&
             $text eq "\n" && $output{$i+1}->{"text"} eq "\n") {
      #print "~~~~~ Next item: " . $output{$tmp}->{"text"};
      # Append the stack trace (if any) for each failed test to the end of its report.
      print_out($text, "none");
      print_out($traces{$test . "\n"}, "none");
      $test = undef;
    } else {
      print_out($text, $color);
    }
  }

  #foreach my $test (keys %traces) {
  #  my $idx = index($output, $test);
  #  if ($idx != -1) {
  #    $idx = index($output, "\n\n", $idx);
  #    substr($output, $idx, 2) = "\n" . $traces{$test} . "\n";
  #  }
  #}
}

print "\nFINAL OUTPUT:\n";
merge_output;
