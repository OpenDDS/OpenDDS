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
my $pub_opts = "";
my $sub_opts = "";
# $pub_opts .= "-DCPSChunks 1 ";

if ($test->{'flags'}->{'low'}) {
  $pub_opts .= "-t 8 -s 128";
  $sub_opts .= "-t 8 -n 1024";

} elsif ($test->{'flags'}->{'medium'}) {
  $pub_opts .= "-t 16 -s 64";
  $sub_opts .= "-t 16 -n 1024";

} elsif ($test->{'flags'}->{'high'}) {
  $pub_opts .= "-t 32 -s 32";
  $sub_opts .= "-t 32 -n 1024";

} elsif ($test->{'flags'}->{'aggressive'}) {
  $pub_opts .= "-t 64 -s 16";
  $sub_opts .= "-t 64 -n 1024";

} elsif ($test->{'flags'}->{'triangle'}) {
  $pub_opts .= "-t 3 -s 3";
  $sub_opts .= "-t 3 -n 9";

} elsif ($test->{'flags'}->{'double'}) {
  $pub_opts .= "-t 2 -s 1";
  $sub_opts .= "-t 2 -n 2";

} elsif ($test->{'flags'}->{'single'}) {
  $pub_opts .= "-t 1 -s 1";
  $sub_opts .= "-t 1 -n 1";

} elsif ($test->{'flags'}->{'superlow'}) {
  $pub_opts .= "-t 4 -s 256";
  $sub_opts .= "-t 4 -n 1024";

} elsif ($test->{'flags'}->{'megalow'}) {
  $pub_opts .= "-t 2 -s 512";
  $sub_opts .= "-t 2 -n 1024";

} else { # default (i.e. lazy)
  $pub_opts .= "-t 1 -s 1024";
  $sub_opts .= "-t 1 -n 1024";
}

if ($test->{'flags'}->{'durable'}) {
  $pub_opts .= " -d";
  $sub_opts .= " -d";
}

if ($debug_level) {
  unlink 'publisher.log' , 'subscriber.log';
  $pub_opts .= " -DCPSDebugLevel $debug_level -DCPSTransportDebugLevel $debug_level -ORBLogFile publisher.log";
  $sub_opts .= " -DCPSDebugLevel $debug_level -DCPSTransportDebugLevel $debug_level -ORBLogFile subscriber.log";
}

my $ini_file = "thrasher.ini";

if ($test->{'flags'}->{'rtps'}) {
  $ini_file = "thrasher_rtps.ini";
}

$pub_opts .= " -DCPSConfigFile $ini_file";
$sub_opts .= " -DCPSConfigFile $ini_file";

if ("thrasher.ini" eq $ini_file) {
  $test->setup_discovery();
}
$test->enable_console_logging();

$test->process('sub', 'subscriber', $sub_opts);
$test->process('pub', 'publisher', $pub_opts);

$test->start_process('sub');
$test->start_process('pub');

exit $test->finish(300);
