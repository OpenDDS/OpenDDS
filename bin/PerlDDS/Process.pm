use PerlACE::Process;

package PerlDDS::Process;

use strict;
use English;
use POSIX qw(:time_h);
use Cwd;

our @ISA = qw(PerlACE::Process);

sub new {
  my $proto = shift;
  my $class = ref ($proto) || $proto;
  my $executable = shift;
  my $arguments = shift;

  my $local_dir = $ENV{"DDS_ROOT"};
  $local_dir =~ s/\/+$//;
  # try and substitute the coverage dir for the local dir
  if($executable !~ /$local_dir/) {
    # since it didn't match the executable must be
    # a relative path so add the cwd
    $executable = Cwd::abs_path($executable);
  }
  my $cov_process = PerlDDS::next_coverage_process();
  my $swap_ext = "/coverage/$cov_process";
  if($executable !~ s/$local_dir/$local_dir$swap_ext/) {
    print STDERR "ERROR: could not swap out $local_dir in $executable for coverage.\n";
  }

  if (defined $ENV{'ACE_TEST_VERBOSE'}) {
    print "INFO: creating coverage executable, process=$cov_process, executable=\n$executable\n\n";
  }

  my $self = PerlACE::Process->new($executable, $arguments);

  $self->{COVERAGE_PROCESS} = $cov_process;
  $self->{TO_SWAP} = $local_dir;
  $self->{SWAP_EXT} = $swap_ext;

  bless($self, $class);
  return $self;
}

sub DESTROY
{
  my $self = shift;
  $self->return_coverage_process();
  $self->SUPER::DESTROY();
}

sub Spawn {
  my $self = shift;
  my $local_dir = $ENV{"DDS_ROOT"};
  PerlDDS::swap_lib_path("$self->{TO_SWAP}$self->{SWAP_EXT}", "$self->{TO_SWAP}");
  my $ret_value = $self->SUPER::Spawn();
  PerlDDS::swap_lib_path("$self->{TO_SWAP}", "$self->{TO_SWAP}$self->{SWAP_EXT}");
  return $ret_value;
}

sub WaitKill ($)
{
  my $self = shift;
  my $timeout = shift;
  my $ret_value = $self->SUPER::WaitKill($timeout);
  $self->return_coverage_process();
  return $ret_value;
}

sub Kill ($)
{
  my $self = shift;
  my $ignore_return_value = shift;
  my $ret_value = $self->SUPER::Kill($ignore_return_value);
  $self->return_coverage_process();
  return $ret_value;
}

sub return_coverage_process
{
  my $self = shift;
  if (defined($self->{COVERAGE_PROCESS})) {
    PerlDDS::return_coverage_process($self->{COVERAGE_PROCESS});
    if (defined $ENV{'ACE_TEST_VERBOSE'}) {
      print "INFO: returned process=$self->{COVERAGE_PROCESS}\n";
    }
    $self->{COVERAGE_PROCESS} = undef;
  }
}

1;
