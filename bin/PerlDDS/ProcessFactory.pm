
package PerlDDS;

use Env (DDS_ROOT);
use strict;
use English;
use POSIX qw(:time_h);
use Cwd;
use PerlACE::TestTarget;

sub create_process {
    my $executable = shift;
    my $arguments = shift;
    # indicates that the created process will be the only process running
    # so coverage can be run on all lone processes
    my $lone_process = shift;
    my $created;

    # identify the test target component extenstion corresponding to the
    # executable (if there is one)
    my $ext = identify_process($executable);
    my $config_name = undef;
    my $os = undef;
    if (defined($ext)) {
        # identify the config name
        $config_name = get_test_target_config_name($ext);
        if (defined($config_name)) {
            $os = get_test_target_os($config_name);
        }
    }

    my $target = PerlDDS::create_test_target($config_name, $os);

    if (defined($target) && defined($target->{IP_ADDRESS}) && $arguments !~ /-DCPSDefaultAddress /) {
        $arguments .= " -DCPSDefaultAddress $target->{IP_ADDRESS}";
    }

    if (defined $target) {
      return $target->CreateProcess($executable, $arguments);
    }

#   if ((PerlACE::is_vxworks_test()) &&
#       (!defined($PerlDDS::vxworks_test_target)) &&
#       (defined($os)) &&
#       ($os =~ /VxWorks/i)) {
#       $PerlDDS::vxworks_test_target =
#           create_test_target($config_name, $os);
#       my @paths = split(':', $PerlDDS::added_lib_path);
#       foreach my $lib_path (@paths) {
#           if ($lib_path ne "") {
#               # make sure the test target has the complete lib path
#               $PerlDDS::vxworks_test_target->AddLibPath(
#                   $lib_path);
#           }
#       }
#       return $PerlDDS::vxworks_test_target->
#           CreateProcess($executable);
#   }

    if ((!PerlDDS::is_coverage_test()) ||
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

sub identify_process {
    my $executable = shift;

    if(match($executable, "DCPSInfoRepo")) {
        return "IR";
    }
    elsif(match($executable, "sub")) {
        return "SUB";
    }
    elsif(match($executable, "pub")) {
        return "PUB";
    }
    elsif(!match($executable, "DCPSInfoRepo") &&
          !match($executable, "sub") &&
          !match($executable, "pub")) {
        return "OTHER";
    }
    return undef;
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
