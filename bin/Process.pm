# $Id$

package PerlDDS::Process;

use strict;
use English;
use POSIX qw(:time_h);

sub create {
  my $executable = shift;
  my $arguments = shift;
  my $created;

  if ((PerlACE::is_vxworks_test()) &&
      (is_process_remote($executable))) {
    $created = new PerlACE::ProcessVX($executable, $arguments);
  }
  elsif ((PerlDDS::is_coverage_test()) &&
         (is_process_remote($executable))) {
    my $cov_executable = $executable;
    my $local_dir = $ENV{"DDS_ROOT"};
    my $cov_dir = $ENV{"COV_DDS_ROOT"};
    # try and substitute the coverage dir for the local dir
    if($cov_executable !~ s/$local_dir/$cov_dir/)
    {
      # since it didn't match the executable must be 
      # a relative path so just add the cov_dir
	  $cov_executable = $cov_dir . '/' . $executable;
    }
    $created = new PerlACE::Process($cov_executable, $arguments);
  }
  else {
    $created = new PerlACE::Process($executable, $arguments);
  }
  return $created;
}

sub is_process_remote {
  my $executable = shift;
  
  # skip all this if we already have a remote process
  # NOTE: may want to move this and log a message if we are trying to start 2
  if(!PerlDDS::remote_process_created())
  {
    if(PerlDDS::is_remote_InfoRepo_test()) {
      if(match($executable, "DCPSInfoRepo")) {
        return 1;
      }
    }
    elsif(PerlDDS::is_remote_sub_test()) {
      if(match($executable, "sub")) {
        return 1;
      }
    }
    elsif(PerlDDS::is_remote_pub_test()) {
      if(match($executable, "pub")) {
        return 1;
      }
    }
    elsif(PerlDDS::is_remote_other_test()) {
      if(!match($executable, "DCPSInfoRepo") &&
         !match($executable, "sub") &&
         !match($executable, "pub")) {
        return 1;
      }
    }
  }
}

sub match {
  my $executable = lc(shift);
  my $to_match = lc(shift);
  return ($executable =~ /$to_match[^\\\/]*$/);
}

sub Spawn {
  my $process = shift;
  my $is_remote = is_process_remote($process->{EXECUTABLE});
  if($is_remote) {
    add_lib_path('$ENV{"COV_DDS_ROOT"}/lib');
  }
  my $ret_value = $process->Spawn();
  if($is_remote) {
    remove_lib_path('$ENV{"COV_DDS_ROOT"}/lib');
  }
  return $ret_value;
}

sub SpawnWaitKill {
  my $process = shift;
  my $seconds = shift;
  my $is_remote = is_process_remote($process->{EXECUTABLE});
  if($is_remote) {
    add_lib_path('$ENV{"COV_DDS_ROOT"}/lib');
  }
  my $ret_value = $process->SpawnWaitKill($seconds);
  if($is_remote) {
    remove_lib_path('$ENV{"COV_DDS_ROOT"}/lib');
  }
  return $ret_value;
}

1;
