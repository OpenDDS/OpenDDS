eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

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

sub generate_rtps_config {
  my ($num_participants) = @_;
  my $config_file;
  my $gen_conf_file;
  if ($test->flag('rtps_disc')) {
    $config_file = "rtps_disc.ini";
  }
  elsif ($test->flag('rtps')) {
    $config_file = "rtps.ini";
  }
  else {
    #not rtps test
    return;
  }
  $gen_conf_file = $config_file;
  $gen_conf_file =~ s/\.ini/_generated\.ini/g;
  print "Config File: $config_file   Gen Config File: $gen_conf_file\n";
  use File::Copy;
  copy($config_file , $gen_conf_file) or die "Copy failed: $!";
  open MYFILE, '+<', $gen_conf_file or die "Open failed: $!";
  my $transport_type_line;
  my $use_multicast_line;
  while(<MYFILE>)  {
    chomp;
    if ($_ =~ /use_multicast=/) {
      $use_multicast_line = $_;
    }
    if ($_ =~ /transport_type=/) {
      $transport_type_line = $_;
    }
  }
  seek MYFILE, 0, 2;
  print MYFILE "\n#START GENERATED TRANSPORT CONFIG";
  my $part_num;
  for($part_num = 0; $part_num < $num_participants; ++$part_num ) {
    print MYFILE "\n\n[config\/domain_part_$part_num]\n";
    print MYFILE "transports=rtps_transport_$part_num\n";
    print MYFILE "\n[transport\/rtps_transport_$part_num]\n";
    print MYFILE "$transport_type_line\n$use_multicast_line";
  }
  close MYFILE;
  $config_opts .= "-DCPSConfigFile $gen_conf_file ";
}

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
my $pub_part = 2;
my $sub_part = 2;
if ($#ARGV < 1) {
    print "no args passed ($#ARGV)\n";
    $pub_processes = 2;
    $sub_processes = 2;
    $config_opts .= '-pub_processes ' . $pub_processes . ' ';
    $config_opts .= '-sub_processes ' . $sub_processes . ' ';
    $config_opts .= '-pub_participants ' . $pub_part . ' ';
    $config_opts .= '-writers 2 ';
    $config_opts .= '-samples ' . $samples . ' ';
    $config_opts .= '-sub_participants ' . $sub_part . ' ';
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
    $pub_part = 1;
    $sub_part = 1;
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
elsif ($ARGV[1] =~ /-\S+_process/ ||
       $ARGV[1] =~ /-\S+_participant/ ||
       $ARGV[1] =~ /-readers/ ||
       $ARGV[1] =~ /-writers/ ||
       $ARGV[1] =~ /-ORBSvcConf/) {
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
    $pub_part = 1;
    if ($config_opts =~ /-pub_participants (\d+)/) {
        $pub_part = $1;
    }
    $sub_part = 1;
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

    if ($#ARGV == 3 && $ARGV[3] eq "orb_csdtp") {
	$config_opts .= '-ORBSvcConf svc_csdtp.conf ';
    }
}
elsif ($config_opts !~ /-sample_size/) {
    $config_opts .= '-sample_size 10 ';
}

if (($test->flag('rtps_disc') || $test->flag('rtps')) &&
    ($sub_part > 1 || $pub_part > 1))
{
  #need to generate proper .ini files for multiple participants in a process
  #each participant requires its own rtps transport
  my $max_parts = $sub_part >= $pub_part ? $sub_part : $pub_part;
  generate_rtps_config($max_parts);
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
    $test->process("subscriber #$index", "subscriber", $sub_opts .
                   ($PerlDDS::SafetyProfile ? "-p $index" : ''));
}
for ($index = 0; $index < $pub_processes; ++$index) {
    $test->process("publisher #$index", "publisher", $pub_opts .
                   ($PerlDDS::SafetyProfile ? '-p '.
                    ($sub_processes + $index) : ''));
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
