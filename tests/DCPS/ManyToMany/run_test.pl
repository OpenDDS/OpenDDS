eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# $Id$
# -*- perl -*-

use Env (DDS_ROOT);
use lib "$DDS_ROOT/bin";
use Env (ACE_ROOT);
use lib "$ACE_ROOT/bin";
use PerlDDS::Run_Test;
use strict;

PerlDDS::add_lib_path('../LargeSample');

my $test = new PerlDDS::TestFramework();

my $status = 0;

$test->{dcps_debug_level} = 0;
$test->{dcps_transport_debug_level} = 0;
$test->{nobits} = 1;
my $pub_opts = "";
my $sub_opts = "";
my $reliable = 1;

my $config_opts = "";
if ($test->flag('udp')) {
    $reliable = 0;
}
elsif ($test->flag('multicast_async')) {
    $config_opts .= "-DCPSConfigFile pub_multicast_async.ini ";
}

$config_opts .= '-reliable ' if $reliable;

sub divides_evenly
{
  my $num = shift;
  my $denom = shift;

  return ($num / $denom) == int($num / $denom);
}

sub delay_msec_calc
{
  my $total_writers = shift;
  my $samples = shift;
  my $base_delay_msec = shift;

  return int(($total_writers * $samples) / 100 + 1) * $base_delay_msec;
}

my $pub_processes = 1;
my $sub_processes = 1;
my $delay_msec;
my $serialized_samples;
my $base_delay_msec = 500;
my $samples = 10;
my $custom = 0;
my $total_writers;
if ($#ARGV < 1) {
    print "no args passed ($#ARGV)\n";
    $pub_processes = 2;
    $sub_processes = 2;
    $config_opts .= '-pub_processes ' . $pub_processes . ' ';
    $config_opts .= '-sub_processes ' . $sub_processes . ' ';
    $config_opts .= '-pub_participants 2 ';
    $config_opts .= '-writers 2 ';
    $config_opts .= '-samples ' . $samples . ' ';
    $config_opts .= '-sub_participants 2 ';
    $config_opts .= '-readers 2 ';
    $delay_msec = $base_delay_msec;
    $serialized_samples = 2 * 2 * 10; # pub_part * writers * samples
}
elsif ($ARGV[1] =~ /(\d+)[t|T][o|O](\d+)/) {
    $total_writers = $1;
    my $total_readers = $2;
    print "total_writers=$total_writers, total_readers=$total_readers\n";
    my $writers = $total_writers;
    my $readers = $total_readers;
    my $pub_part = 1;
    my $sub_part = 1;
    if (divides_evenly($writers, 2)) {
        $pub_part = 2;
        $writers /= 2;
    }
    if (divides_evenly($readers, 2)) {
        $sub_part = 2;
        $readers /= 2;
    }
    $config_opts .= '-pub_participants ' . $pub_part . ' ';
    $config_opts .= '-sub_participants ' . $sub_part . ' ';

    if (divides_evenly($writers, 2)) {
        $pub_processes = 2;
        $writers /= 2;
    }
    if (divides_evenly($readers, 2)) {
        $sub_processes = 2;
        $readers /= 2;
    }
    $config_opts .= '-pub_processes ' . $pub_processes . ' ';
    $config_opts .= '-sub_processes ' . $sub_processes . ' ';

    $config_opts .= '-writers ' . $writers . ' ';
    $config_opts .= '-samples ' . $samples . ' ';
    $config_opts .= '-readers ' . $readers . ' ';
    # produce at most 100 samples per 500ms
    $delay_msec = delay_msec_calc($total_writers, $samples, $base_delay_msec);
    $serialized_samples = $pub_part * $writers * $samples;
}
elsif ($ARGV[1] =~ /-\S_process/ ||
       $ARGV[1] =~ /-\S_participant/ ||
       $ARGV[1] =~ /-readers/ ||
       $ARGV[1] =~ /-writers/) {
    $custom = 1;
    my $added_opts = $ARGV[1];
    $added_opts =~ s/-reliable//;
    $config_opts .= $added_opts . ' ';
    if ($config_opts =~ /-pub_processes (\d+)/) {
        $pub_processes = $1;
    }
    if ($config_opts =~ /-sub_processes (\d+)/) {
        $sub_processes = $1;
    }
    my $pub_part = 1;
    if ($config_opts =~ /-pub_participants (\d+)/) {
        $pub_part = $1;
    }
    my $sub_part = 1;
    if ($config_opts =~ /-sub_participants (\d+)/) {
        $sub_part = $1;
    }
    if ($config_opts =~ /-samples (\d+)/) {
        $samples = $1;
    }
    my $writers = 1;
    if ($config_opts =~ /-writers (\d+)/) {
        $writers = $1;
    }
    my $readers = 1;
    if ($config_opts =~ /-readers (\d+)/) {
        $readers = $1;
    }
    if ($config_opts =~ /-delay_msec (\d+)/) {
        $delay_msec = $1;
    } else {
	$delay_msec = delay_msec_calc($total_writers, $samples, $base_delay_msec);
    }

    # only used if -total_duration_msec is not in $ARGV[1]
    $serialized_samples = $pub_part * $writers * $samples;
}
else {
    print STDERR "ERROR: invalid argv[1]=$ARGV[1]\n";
}

if ($config_opts !~ /-delay_msec/) {
    $config_opts .= '-delay_msec ' . $delay_msec . ' ';
}
# double the total time (in seconds) it takes for the publishers
# to send the data
my $total_duration_msec;
if ($config_opts =~ /-total_duration_msec (\d+)/) {
    $total_duration_msec = $1;
}
else {
    $total_duration_msec = $serialized_samples * $delay_msec * 2;
    $config_opts .= '-total_duration_msec ' . $total_duration_msec . ' ';
}

my $min_total_duration_msec = 60000;
if ($total_duration_msec < $min_total_duration_msec) {
    $total_duration_msec = $min_total_duration_msec;
    $config_opts =~ s/(-total_duration_msec) (\d+)/$1 $total_duration_msec/;
}

if (!$custom) {
    if ($#ARGV < 2 || $ARGV[2] eq  "small") {
        $config_opts .= '-sample_size 10 ';
    }
    elsif ($ARGV[2] eq  "large") {
        $config_opts .= '-sample_size 66000 ';
    }
    elsif ($ARGV[2] eq  "medium") {
        $config_opts .= '-sample_size 33000 ';
    }
    elsif ($ARGV[2] =~ /^\d+$/) {
        $config_opts .= '-sample_size ' . $ARGV[2] . ' ';
    }
}
elsif ($config_opts !~ /-sample_size/) {
    $config_opts .= '-sample_size 10 ';
}

$test->default_transport("tcp");

my $index;
print "config_opts=$config_opts\n";
$pub_opts .= $config_opts;
print "pub_opts=$pub_opts\n";
$sub_opts .= $config_opts;
print "sub_opts=$sub_opts\n";

$test->setup_discovery();

for ($index = 0; $index < $sub_processes; ++$index) {
    $test->process("subscriber #$index", "subscriber", $sub_opts);
}
for ($index = 0; $index < $pub_processes; ++$index) {
    $test->process("publisher #$index", "publisher", $pub_opts);
}

for ($index = 0; $index < $pub_processes; ++$index) {
    $test->start_process("publisher #$index");
}

for ($index = 0; $index < $sub_processes; ++$index) {
    $test->start_process("subscriber #$index");
}

# first subscriber process needs to be killed a little after the
# total expected duration
my $wait_to_kill = $total_duration_msec * 2;
print "wait_to_kill=$wait_to_kill\n";
# ignore this issue that is already being tracked in redmine
$test->ignore_error("(Redmine Issue# 1446)");
exit $test->finish($wait_to_kill);
