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

my $status = 0;

my $logging_p = "";#"-DCPSDebugLevel 1 -ORBVerboseLogging 1 " .
#    "-DCPSTransportDebugLevel 1";#6 -DCPSDebugLevel 10";
my $logging_s = "";#"-DCPSDebugLevel 1 -ORBVerboseLogging 1 " .
#    "-DCPSTransportDebugLevel 1";#6 -DCPSDebugLevel 10";
my $pub_opts = "$logging_p -ORBLogFile pubx.log ";
my $sub_opts = "$logging_s -ORBLogFile subx.log ";
my $repo_bit_opt = '';
my $reliable = 1;

my $nobit = 1; # Set to a non-zero value to disable the Builtin Topics.
my $app_bit_opt = '-DCPSBit 0 ' if $nobit;
$repo_bit_opt = '-NOBITS' if $nobit;

my $config_opts = "";
if ($#ARGV < 0 || $ARGV[0] eq 'tcp') {
    $config_opts .= ' -DCPSConfigFile tcp.ini ';
}
elsif ($ARGV[0] eq 'udp') {
    $config_opts .= "$app_bit_opt -DCPSConfigFile udp.ini ";
    $reliable = 0;
}
elsif ($ARGV[0] eq 'multicast') {
    $config_opts .= "$app_bit_opt -DCPSConfigFile multicast.ini ";
}
elsif ($ARGV[0] eq 'multicast_async') {
    $config_opts .= "$app_bit_opt -DCPSConfigFile pub_multicast_async.ini ";
}
elsif ($ARGV[0] eq 'shmem') {
    $config_opts .= "$app_bit_opt -DCPSConfigFile shmem.ini ";
}
elsif ($ARGV[0] eq 'rtps') {
    $config_opts .= "$app_bit_opt -DCPSConfigFile rtps.ini ";
}
elsif ($ARGV[0] ne '') {
    print STDERR "ERROR: invalid test case\n";
    exit 1;
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

my $dcpsrepo_ior = "repo.ior";

unlink $dcpsrepo_ior;
unlink <*.log>;

my $DCPSREPO = PerlDDS::create_process("$ENV{DDS_ROOT}/bin/DCPSInfoRepo",
                                       "$repo_bit_opt -o $dcpsrepo_ior");

my $index;
print "config_opts=$config_opts\n";
$pub_opts .= $config_opts;
print "pub_opts=$pub_opts\n";
$sub_opts .= $config_opts;
print "sub_opts=$sub_opts\n";
my @Subscriber;
my @Publisher;
for ($index = 0; $index < $sub_processes; ++$index) {
    my $temp_opts = $sub_opts;
    $temp_opts =~ s/sub(x)\.log/sub$index\.log/;
    push(@Subscriber, PerlDDS::create_process("subscriber", $temp_opts));
}
for ($index = 0; $index < $pub_processes; ++$index) {
    my $temp_opts = $pub_opts;
    $temp_opts =~ s/pub(x)\.log/pub$index\.log/;
    push(@Publisher, PerlDDS::create_process("publisher", $temp_opts));
}

print $DCPSREPO->CommandLine() . "\n";
$DCPSREPO->Spawn();
if (PerlACE::waitforfile_timed($dcpsrepo_ior, 30) == -1) {
    print STDERR "ERROR: waiting for Info Repo IOR file\n";
    $DCPSREPO->Kill();
    exit 1;
}

for ($index = 0; $index < $pub_processes; ++$index) {
    print $Publisher[$index]->CommandLine() . "\n";
    $Publisher[$index]->Spawn();
}

for ($index = 0; $index < $sub_processes; ++$index) {
    print $Subscriber[$index]->CommandLine() . "\n";
    $Subscriber[$index]->Spawn();
}

# first subscriber process needs to be killed a little after the
# total expected duration
my $wait_to_kill = $total_duration_msec * 2;
print "wait_to_kill=$wait_to_kill\n";
for ($index = 0; $index < $sub_processes; ++$index) {
    my $SubscriberResult = $Subscriber[$index]->WaitKill($wait_to_kill);
    if ($SubscriberResult != 0) {
        print STDERR "ERROR: subscriber[$index] returned $SubscriberResult\n";
        $status = 1;
    }
    $wait_to_kill = 10;
}

for ($index = 0; $index < $pub_processes; ++$index) {
  my $PublisherResult = $Publisher[$index]->WaitKill(10);
  if ($PublisherResult != 0) {
      print STDERR "ERROR: publisher[$index] returned $PublisherResult\n";
      $status = 1;
  }
}

my $ir = $DCPSREPO->TerminateWaitKill(10);
if ($ir != 0) {
    print STDERR "ERROR: DCPSInfoRepo returned $ir\n";
    $status = 1;
}

unlink $dcpsrepo_ior;

if ($status == 0) {
  print "test PASSED.\n";
} else {
  print "**** Begin log file output *****\n";
  for ($index = 0; $index < $pub_processes; ++$index) {
      if (open FILE, "<", "pub$index.log") {
          print "Publisher[$index]:\n";
          while (my $line = <FILE>) {
              print "$line";
          }
          print "\n\n";
          close FILE;
      }
  }

  for ($index = 0; $index < $sub_processes; ++$index) {
      if (open FILE, "<", "sub$index.log") {
          print "Subscriber[$index]:\n";
          while (my $line = <FILE>) {
              print "$line";
          }
          print "\n\n";
          close FILE;
      }
  }
  print "**** End log file output *****\n";
  print STDERR "test FAILED.\n";
}

exit $status;
