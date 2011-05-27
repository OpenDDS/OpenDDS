#!/usr/bin/perl -w

# ******************************************************************
# Pragma Section
# ******************************************************************

use strict;
use Cwd;
use FileHandle;
use File::Basename;
use Getopt::Long;

# ******************************************************************
# Subroutine Section
# ******************************************************************

sub ProcessOptions {
  my($name)    = shift;
  my($options) = shift;
  my($desc)    = shift;

  if (defined $options) {
    my($result)  = GetOptions(%$options);
    my($usage)   = undef;

    ## Find the help option
    foreach my $key (keys %$options) {
      if ($key eq "help") {
        $usage = $$options{$key};
        last;
      }
    }

    ## Check for usage
    if ((defined $usage && defined $$usage) || !$result) {
      usageAndExit($name, $options, $desc);
    }
  }
}


sub usageAndExit {
  my($name)    = shift;
  my($options) = shift;
  my($desc)    = shift;
  my($str)     = "Usage: " . basename($name);
  my($initial) = length($str);
  my($length)  = $initial;
  my($maxLine) = 78;

  print $str;

  foreach my $key (sort keys %$options) {
    my($opt, $type) = split(/=/, $key);
    my($str) = " [--$opt" . (defined $type ? " <$opt>" : "") . "]";
    my($len) = length($str);
    if ($length + $len > $maxLine) {
      print "\n" . (" " x $initial);
      $length = $initial;
    }
    print $str;
    $length += $len;
  }
  print "\n";

  if (defined $desc) {
    my($max) = 0;
    foreach my $key (sort keys %$desc) {
      my($len) = length($key);
      if ($len > $max) {
        $max = $len;
      }
    }
    if ($max > 0) {
      print "\n";
    }

    foreach my $key (sort keys %$desc) {
      my($len) = length($key);
      my($str) = "       $key" . ($len < $max ? " " x ($max - $len) : "") .
                 "  ";
      $length = length($str);

      print $str;

      $length++;
      if (length($$desc{$key}) + $length > $maxLine) {
        my($part) = $$desc{$key};
        while(length($part) + $length > $maxLine) {
          my(@words) = split(/\s+/, $part);
          $part = "";
          foreach my $word (@words) {
            if (length($word) + length($part) + $length > $maxLine) {
              my($space) = "         " . (" " x $max);
              $part =~ s/\s+$//;
              print "$part\n$space";
              $part = "";
              $length = length($space) + 1;
            }
            $part .= "$word ";
          }
        }
        $part =~ s/\s+$//;
        print "$part.\n";
      }
      else {
        print "$$desc{$key}.\n";
      }
    }
  }
  exit(0);
}

sub checkSkip {
  my($file) = shift;
  my($fd)   = new FileHandle();

  if (open($fd, $file)) {
    while(<$fd>) {
      my($line) = $_;
      $line =~ s/#.*//;
      $line =~ s/^\s+//;
      $line =~ s/\s+$//;
      if ($^O =~ /$line/i) {
        return 1;
      }
    }
    close($fd);
  }
  return 0;
}


sub getTestScripts {
  my($dir)     = shift;
  my(@scripts) = ();
  my($dh)      = new FileHandle();
  if (opendir($dh, $dir)) {
    foreach my $file (sort grep(!/^\.\.?$/, readdir($dh))) {
      my($full) = "$dir/$file";
      if (-d $full) {
        push(@scripts, getTestScripts($full));
      }
      else {
        if ($file =~ /^run.*\.pl$/) {
          push(@scripts, $full);
        }
      }
    }
    closedir($dh);
  }
  return @scripts;
}

sub cleanDirectory {
  my($dir) = shift;
  my($dh)  = new FileHandle();
  if (opendir($dh, $dir)) {
    foreach my $file (sort grep(!/^\.\.?$/, readdir($dh))) {
      my($full) = "$dir/$file";
      unlink($full)
    }
    closedir($dh);
  }
}


sub terminalSupportsSpecial {
  return (-t 1 && defined $ENV{TERM} &&
          ($ENV{TERM} =~ /xterm/ || $ENV{TERM} =~ /rxvt/));
}


sub generateHTML {
  my($base)     = shift;
  my($testInfo) = shift;
  my($fh)       = new FileHandle();

  if (open($fh, ">$base.html")) {
    print $fh "<html>\n" .
              "<table border=4 cellpadding=3 cellspacing=1>\n" .
              "  <tr bgcolor=#4f94cd>\n" .
              "    <th>Test</th><th>Skipped</th><th>Passed</th>" .
              "<th>Failed</th><th>Total</th>\n" .
              "  </tr>\n";
    foreach my $key (sort keys %$testInfo) {
      print $fh "  <tr>\n" .
                "    <td align=left>$key</td>" .
                "<td align=right>$$testInfo{$key}->[0]</td>" .
                "<td align=right>$$testInfo{$key}->[1]</td>" .
                "<td align=right>$$testInfo{$key}->[2]</td>" .
                "<td align=right>$$testInfo{$key}->[3]</td>\n" .
                "  </tr>\n";
    }
    print $fh "</table>\n";
    close($fh);
  }
}

# ******************************************************************
# Main Section
# ******************************************************************

my($base)    = basename($0);
my($usage)   = undef;
my($loop)    = 0;
my($count)   = 0;
my($html)    = 0;
my($debug)   = 0;
my($verbose) = 0;
my($only)    = undef;
my($winDebug)  = 0;
my($nodir)   = 0;
my(%options) = ("help"    => \$usage,
                "loop"    => \$loop,
                "count=i" => \$count,
                "html"    => \$html,
                "debug"   => \$debug,
                "verbose" => \$verbose,
                "only=s"  => \$only,
                                "winDebug"  => \$winDebug,
               );
my(%desc)    = ("loop"  => "Run the tests in a loop until a failure is " .
                           "seen",
                "count" => "This option is similar to --loop, except " .
                           "that it loops <count> times pass or fail.  " .
                           "This option automatically enables the " .
                           "--html option",
                "html"  => "Generate $base.html at the end which " .
                           "contains the details of the test run",
                "debug" => "Pass the --debug option all tests",
                "verbose" => "Pass the --verbose option all tests",
                "only"  => "Only run this specific test",
                                "winDebug" => "Pass the --winDebug option all tests",
               );
ProcessOptions($0, \%options, \%desc);

if ($count > 0) {
  $html = 1;
}

my($startDir) = getcwd();
my($logDir)   = $startDir . "/log";

my($startDirRegexp) = $startDir;
$startDirRegexp =~ s/([\+\-])/\\$1/g;

mkdir($logDir, 0755);

my($loopCount)   = 0;
my($run)         = 1;
my($totalfailed) = 0;
my(%testInfo)    = ();

$SIG{INT} = sub { chdir($startDir);
                  if ($html) {
                    generateHTML($base, \%testInfo);
                  }
                  exit(2);
                };

$| = 1;
my @failedTests;
my $i = 0;
do {
  $loopCount++;
  cleanDirectory($logDir);
  print "Running all tests: " . scalar(localtime(time())) . "\n";

  my($totalpassed)  = 0;
  my($totalskipped) = 0;
  $totalfailed      = 0;

  my(@scripts) = (defined $only ? $only : getTestScripts($startDir));
  foreach my $script (@scripts) {
    my($fh)     = new FileHandle();
    my($oh)     = new FileHandle();
    my($dname)  = dirname($script);
    my($bname)  = basename($script);
    my($skip) = 0;
    my($status) = 0;
    my($nodir)  = $script;
    $nodir =~ s/$startDirRegexp\///;

    my($starttime) = time();
    print "$nodir: ";

    if ($run) {
      if (-r "$dname/.skip.$bname") {
        $skip = checkSkip("$dname/.skip.$bname");
      }
      if (!$skip) {
        ## Run the perl script and redirect the output
        chdir($dname);
        if (open($fh, "$^X $bname " . ($debug ? "--debug " : "") .
                                              ($winDebug ? "--winDebug " : "") .
                                      ($verbose ? "--verbose " : "") . "2>&1 |")) {
          my($logFile) = $nodir . ".txt";
          $logFile =~ s/\//_/g;

          if (open($oh, ">$logDir/$logFile")) {
            $status = 1;
            while(<$fh>) {
              print $oh $_;
              if (/ERROR:/ || /EXCEPTION/i) {
                $status = 0;
              }
            }
            close($oh);
          }
          close($fh);
          if ($? ne 0) {
            $status = 0;
          }
        }
      }
    }

    ## Set up the hash value array
    if (!defined $testInfo{$script}) {
      $testInfo{$script} = [0, 0, 0, 0];
    }

    ++$testInfo{$script}->[3];

    ## Print the pass/fail output
    if ($skip) {
      print "Skipped\n";
      $totalskipped++;
      ++$testInfo{$script}->[0];
    } elsif ($status) {
      my($elapsedtime) = time() - $starttime;
      print "Passed ($elapsedtime)\n";
      $totalpassed++;
      ++$testInfo{$script}->[1];
    } else {
      if (terminalSupportsSpecial()) {
        print "\x1b[1mFailed\x1b[m\n";
        $failedTests[$i]= $nodir."\n";
                $i++;
      }
      else {
        $failedTests[$i]= $nodir."\n";
                print "** Failed **\n";
                $i++;
      }
      $totalfailed++;
      ++$testInfo{$script}->[2];
    }
  }
  print "All tests completed: " . scalar(localtime(time())) . "\n" .
        "$totalpassed tests passed\n" .
        "$totalfailed tests failed\n" .
        "$totalskipped tests skipped\n\n";
        if ($totalfailed > 0) {
          print "Tests that failed are:\n";
          print " @failedTests";
        }

  if ($loop) {
    print "Finished $loopCount sequence" . (($loopCount != 1) ? "s" : "") .
          " of tests\n";
  }
} while(($count > 0 && $loopCount < $count) || ($loop && $totalfailed == 0));

if ($html) {
  chdir($startDir);
  generateHTML($base, \%testInfo);
}
