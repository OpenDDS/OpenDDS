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
  my $created;

  if ((PerlACE::is_vxworks_test()) &&
      (is_process_special($executable))) {
    # until we figure out how we want to handle rtp, only
    # allow one process to be on VxWorks
    PerlDDS::special_process_created();
    $created = new PerlACE::ProcessVX($executable, $arguments);
  }
  elsif ((!PerlDDS::is_coverage_test()) ||
         (non_dds_test($executable)) ||
         (is_process_special($executable))){
    if(PerlDDS::is_coverage_test())
    {
      PerlDDS::special_process_created();
    }
    $created = new PerlACE::Process($executable, $arguments);
  }
  elsif (PerlDDS::is_coverage_test()) {
    print STDOUT "Remote Process \n";
    my $cov_executable = $executable;
    my $local_dir = $ENV{"DDS_ROOT"};
    my $cov_dir = $ENV{"COV_DDS_ROOT"};
    # try and substitute the coverage dir for the local dir
    if($cov_executable !~ /$local_dir/)
    {
      # since it didn't match the executable must be 
      # a relative path so add the cwd
      $cov_executable = getcwd() . '/' . $executable;
    }
    if($cov_executable !~ s/$local_dir/$cov_dir/)
    {
      print STDERR "This shouldn't be reached, something wrong with the cwd \n";
    }
    print STDOUT "cov_exe=$cov_executable \n";
    $created = new PerlDDS::Process($cov_executable, $arguments);
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
    my $inforepo = PerlDDS::is_special_InfoRepo_test();
    my $pub = PerlDDS::is_special_pub_test();
    my $sub = PerlDDS::is_special_sub_test();
    my $other = PerlDDS::is_special_other_test();
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
