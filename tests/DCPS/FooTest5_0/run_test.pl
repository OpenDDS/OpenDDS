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

PerlDDS::add_lib_path('../FooType4');

# single reader with single instances test
my $multiple_instance = 0;
my $num_samples_per_reader = 3;
my $num_readers = 1;
my $use_take = 0;

my $dcpsrepo_ior = 'repo.ior';

unlink $dcpsrepo_ior;

my $DCPSREPO = PerlDDS::create_process("$DDS_ROOT/bin/DCPSInfoRepo",
                                       "-o $dcpsrepo_ior");

my $parameters = "-DCPSConfigFile all.ini -r $num_readers -t $use_take"
               . " -m $multiple_instance -i $num_samples_per_reader";

if ($ARGV[0] eq 'udp') {
  $parameters .= ' -us -up';
}
elsif ($ARGV[0] eq 'diff_trans') {
  $parameters .= ' -up';
}
elsif ($ARGV[0] eq 'rtps') {
  $parameters .= ' -rs -rp';
}
my $FooTest5 = PerlDDS::create_process('main', $parameters);

print $DCPSREPO->CommandLine(), "\n";
$DCPSREPO->Spawn();

if (PerlACE::waitforfile_timed($dcpsrepo_ior, 30) == -1) {
  print STDERR "ERROR: waiting for DCPSInfo IOR file\n";
  $DCPSREPO->Kill();
  exit 1;
}

print $FooTest5->CommandLine(), "\n";
$FooTest5->Spawn();

my $result = $FooTest5->WaitKill(60);

my $status = 0;
if ($result != 0) {
  print STDERR "ERROR: main returned $result\n";
  $status = 1;
}


my $ir = $DCPSREPO->TerminateWaitKill(5);

if ($ir != 0) {
  print STDERR "ERROR: DCPSInfoRepo returned $ir\n";
  $status = 1;
}

if ($status == 0) {
  print "test PASSED.\n";
}
else {
  print STDERR "test FAILED.\n";
}
exit $status;
