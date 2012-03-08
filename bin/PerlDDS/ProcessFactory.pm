# $Id$

package PerlDDS;

use Env (DDS_ROOT);
use strict;
use English;
use POSIX qw(:time_h);
use Cwd;

sub create_process {
  my $executable = shift;
  my $arguments = shift;
  # indicates that the created process will be the only process running
  # so coverage can be run on all lone processes
  my $lone_process = shift;
  my $created;

  if ((PerlACE::is_vxworks_test()) &&
      (is_process_special($executable))) {
    # until we figure out how we want to handle rtp, only
    # allow one process to be on VxWorks
    PerlDDS::special_process_created();
    # NOTE: $lone_process was added for coverage, but it may allow us to
    # create more than one special process in a run
    $created = new PerlACE::ProcessVX($executable, $arguments);
  }
  elsif ((!PerlDDS::is_coverage_test()) ||
         (non_dds_test($executable))) {
    $created = new PerlACE::Process($executable, $arguments);
  }
  elsif (PerlDDS::is_coverage_test()) {
    $created = new PerlDDS::Process($executable, $arguments);
  }
  else {
    print STDERR "This shouldn't be reached, no Process created \n";
  }
  return $created;
}

sub is_process_special {
  my $executable = shift;

  # skip all this if we already have a special process
  # NOTE: may want to move this and log a message if we are trying to start 2
  if(!PerlDDS::is_special_process_created())
  {
    if(PerlDDS::is_special_InfoRepo_test()) {
      if(match($executable, "DCPSInfoRepo")) {
        return 1;
      }
    }
    elsif(PerlDDS::is_special_sub_test()) {
      if(match($executable, "sub")) {
        return 1;
      }
    }
    elsif(PerlDDS::is_special_pub_test()) {
      if(match($executable, "pub")) {
        return 1;
      }
    }
    elsif(PerlDDS::is_special_other_test()) {
      if(!match($executable, "DCPSInfoRepo") &&
         !match($executable, "sub") &&
         !match($executable, "pub")) {
        return 1;
      }
    }
  }
  return 0;
}

sub match {
  my $executable = lc(shift);
  my $to_match = lc(shift);
  return ($executable =~ /$to_match[^\\\/]*$/);
}

sub non_dds_test {
  my $executable = shift;
  my $fp_executable = Cwd::abs_path($executable);
  if($fp_executable !~ /$DDS_ROOT/)
  {
    print STDOUT "non DDS process, $executable\n";
    return 1;
  }
  return 0;
}

1;
