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

my @common_opts = ();
my @common_pub_opts = ();
my @sub_opts = ();
my $reliable = 1;
my $is_rtps = 0;
my $writer_process_count = 2;
my $writers_per_process = 2;
my $samples_per_writer = 10;
my $data_length_offset = 0;
my $security_id = 1;

my $test = new PerlDDS::TestFramework();

# let TestFramework handle ini file, but also need to identify that
# we are using a non-reliable transport
if ($test->flag('udp')) {
  $reliable = 0;
}
# cannot use default ini for multicast_async
elsif ($test->flag('multicast_async')) {
  push(@common_pub_opts, "-DCPSConfigFile", "pub_multicast_async.ini");
  push(@sub_opts, "-DCPSConfigFile", "multicast.ini");
}
elsif ($test->flag('rtps')) {
  $is_rtps = 1;
  push(@common_opts,
    "-DCPSConfigFile", "rtps.ini",
  );
}
elsif ($test->flag('rtps_disc_sec')) {
  $is_rtps = 1;
  push(@common_opts,
    "-DCPSConfigFile", "rtps_disc_sec.ini",
    "-DCPSSecurityDebugLevel", "1",
  );
}
push(@common_opts, "-r $reliable -w $writers_per_process -s $samples_per_writer -o $data_length_offset");

push(@common_pub_opts, @common_opts);
push(@sub_opts, @common_opts, "-p", "$writer_process_count");

$test->report_unused_flags();
if (!$is_rtps) {
  # use tcp if no transport is set on command line
  $test->default_transport("tcp");
  $test->setup_discovery("-ORBDebugLevel 1 -ORBLogFile DCPSInfoRepo.log -DCPSDebugLevel 1");
}

$test->process("subscriber", "subscriber", join(' ', @sub_opts, '-i', $security_id++));
for (my $pub = 0; $pub < $writer_process_count; $pub++) {
  my $num = $pub + 1;
  my $name = "publisher #$num";
  my @pub_opts = ();
  push(@pub_opts, @common_pub_opts, '-i', $security_id++);
  push(@pub_opts, "-p", "$num") if ($PerlDDS::SafetyProfile);
  $test->process($name, "publisher", join(' ', @pub_opts));
  $test->start_process($name);
}
$test->start_process("subscriber");

# ignore this issue that is already being tracked in redmine
$test->ignore_error("(Redmine Issue# 1446)");
exit $test->finish(100) ? 1 : 0;
