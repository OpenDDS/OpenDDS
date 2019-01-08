eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# -*- perl -*-

use lib "$ENV{ACE_ROOT}/bin";
use lib "$ENV{DDS_ROOT}/bin";
use PerlDDS::Run_Test;
use strict;

my $status = 0;

sub which {
  my $file = shift;
  my $exeext = ($^O eq 'MSWin32') ? '.exe' : '';
  for my $p (File::Spec->path()) {
    if (-x "$p/$file") {
      return "$p/$file";
    }
    elsif ($exeext ne '' && -x "$p/$file$exeext") {
      return "$p/$file$exeext";
    }
  }
  return undef;
}

my $test = new PerlDDS::TestFramework();
$test->{dcps_debug_level} = 0;
$test->{dcps_transport_debug_level} = 0;
$test->{add_orb_log_file} = 0;

$test->process('receiver', which("node"), "./receiver.js --group 224.0.0.4:4000");
$test->start_process('receiver');

$test->process('repeater1', which("node"), "../repeater.js --group 224.0.0.4:4000 --uport 4001 --send 127.0.0.1:5001");
$test->start_process('repeater1');

$test->process('repeater2', which("node"), "../repeater.js --group 224.0.0.5:5000 --uport 5001 --send 127.0.0.1:4001");
$test->start_process('repeater2');

sleep(5);

$test->process('sender', which("node"), "./sender.js --group 224.0.0.5:5000");
$test->start_process('sender');

sleep(10);

$test->kill_process(1, 'repeater1');
$test->kill_process(1, 'repeater2');

$test->stop_process(1, 'sender');
$test->stop_process(1, 'receiver');

exit $test->finish();
