#!/usr/bin/perl

use strict; use warnings;

use HTML::Parser;
use Term::ANSIColor;

use HTML::Parser 3.00 ();

my %inside;
my %output;
my $line_count = 0;
my %traces;

sub tag {
  my($tag, $num) = @_;
  $inside{$tag} += $num;
  if ($tag =~ /h[1-3]/) {
    $output{$line_count++} = {"color" => "none", "text" => "\n"}; # not for all tags
  }
  if ($tag eq "br") {
    $output{$line_count++} = {"color" => "none", "text" => "\n"}; # not for all tags
  }
  if ($tag eq "body" && $num < 0) {
    $output{$line_count++} = {"color" => "none", "text" => "\n"}; # not for all tags
  }
}

sub text {
  return if $inside{script} || $inside{style};
  my $esc = 1;
  my $color;
  if ( $inside{h2}) {
    print color 'yellow';
    $color = 'yellow';
  }
  elsif ( $inside{h3}) {
    print color 'red';
    $color = 'red';
  }
  else {
    $color = "none";
    $esc = 0;
  }
  my $line = $_[0];
  $line =~ s/\R//g;
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
  close($fh);
}

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
  # First argument is a brief html output
  parse_html(shift);

  # Optional second argument is a full text output
  my $full_log = shift;
  if (defined $full_log) {
    collect_stack_trace($full_log);
  }

  my $full_log_file = shift;
  my $test;
  foreach my $i (sort {$a <=> $b} keys %output) {
    my $color = $output{$i}->{"color"};
    my $text = $output{$i}->{"text"};
    if ($color eq "red") {
      $test = $text;
      print_out($text, "red");
    } elsif (defined $test && exists $traces{$test . "\n"} &&
             $text eq "\n" && $output{$i+1}->{"text"} eq "\n") {
      # Append the stack trace (if any) for each failed test to the end of its report.
      print_out($text, "none");
      print_out($traces{$test . "\n"}, "none");
      $test = undef;
    } else {
      print_out($text, $color);
    }
  }
}

merge_output(shift, shift);
