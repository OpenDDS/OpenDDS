eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# -*- perl -*-

use strict;
use warnings;

use Env qw(DDS_ROOT ACE_ROOT);
use lib "$DDS_ROOT/bin";
use lib "$ACE_ROOT/bin";
use PerlDDS::Run_Test;

PerlDDS::add_lib_path('../FooType');

my $test = new PerlDDS::TestFramework();

my $debug_level = 0;
my $opts = "";
# $opts .= "-DCPSChunks 1 ";

if ($test->{'flags'}->{'low'}) {
  $opts .= "-t 8 -s 128 -n 1024";
} elsif ($test->{'flags'}->{'medium'}) {
  $opts .= "-t 16 -s 64 -n 1024";
} elsif ($test->{'flags'}->{'high'}) {
  $opts .= "-t 32 -s 32 -n 1024";
} elsif ($test->{'flags'}->{'aggressive'}) {
  $opts .= "-t 64 -s 16 -n 1024";
} elsif ($test->{'flags'}->{'triangle'}) {
  $opts .= "-t 3 -s 3 -n 9";
} elsif ($test->{'flags'}->{'double'}) {
  $opts .= "-t 2 -s 1 -n 2";
} elsif ($test->{'flags'}->{'single'}) {
  $opts .= "-t 1 -s 1 -n 1";
} elsif ($test->{'flags'}->{'superlow'}) {
  $opts .= "-t 4 -s 256 -n 1024";
} elsif ($test->{'flags'}->{'megalow'}) {
  $opts .= "-t 2 -s 512 -n 1024";
} else { # default (i.e. lazy)
  $opts .= "-t 1 -s 1024 -n 1024";
}

if ($test->{'flags'}->{'durable'}) {
  $opts .= " -d";
}

if ($debug_level) {
  unlink 'Thrasher.log';
  $opts .= " -DCPSDebugLevel $debug_level -DCPSTransportDebugLevel $debug_level -ORBLogFile Thrasher.log";
}

my $ini_file = "thrasher.ini";
if ($test->{'flags'}->{'rtps'}) {
  $ini_file = "thrasher_rtps.ini";
}
$opts .= " -DCPSConfigFile $ini_file";
if ("thrasher.ini" eq $ini_file) {
  $test->setup_discovery();
}

$test->enable_console_logging();
$test->process('Thrasher', 'Thrasher', $opts);
$test->start_process('Thrasher');

exit $test->finish(600);
