# This script is purposed to install the build of Bench to a destination of choice provided on the
# command line with the --dest paramenter. At the destination chosen, a 'bin' and 'lib' directory
# is created with a test_controller, node_controller, and worker from the build copied to 'bin'.
# The libs from the various support libraries of Bench are collected from the build tree of Bench
# and copied over to the 'lib' directory created in the destination. Subsequent run of Bench by a
# user session will need to add the {destination path}/bin to the PATH environment and the
# {destination path}/lib to the LD_LIBRARY_PATH prior to running a Bench test.
# IMPORTANT: Since this is taken from an OpenDDS build, the 'DDS_ROOT' environment variable must
# set prior to running this script.


eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# -*- perl -*-

use File::Path qw(make_path);
use Getopt::Long;
use File::Find;
use File::Copy "cp";

die "ERROR: DDS_ROOT not set. Set DDS_ROOT to the OpenDDS installation directiory and rerun this script.\n"
    unless defined($ENV{'DDS_ROOT'});

my $from = "$ENV{'DDS_ROOT'}/performance-tests/bench_2";
my $dest;
GetOptions ("dest=s" => \$dest)
  or die("USAGE: perl install_bench.pl --dest /path/to/bench/root \n");

die("USAGE: perl install_bench.pl --dest /path/to/bench/root \n") unless defined $dest;

# Create destination directories if they don't exist


make_path( "${dest}/bin", "${dest}/lib" );

my @bins = ('node_controller/node_controller',
            'test_controller/test_controller',
            'worker/worker');

foreach (@bins) {
  my $fn = (split '/', $_)[-1];
  cp("$from/$_", "$dest/bin");
  chmod 0755, "$dest/bin/$fn";
}

# Collect and copy libs to destination
# Also create symlinks for each lib copied over

my @libs;

find( sub { /^.*\.so\..*\z/s && push @libs, $File::Find::name }, "$from" );
foreach (@libs) {
    my $fn = (split '/', $_)[-1];
    my $sln = (split '\.',$fn)[0];
    unlink "$dest/lib/${sln}.so";
    unlink "$dest/lib/$fn";
    cp("$_", "$dest/lib");
    chmod 0755, "$dest/lib/$fn";
    symlink("$dest/lib/$fn", "$dest/lib/${sln}.so") || die "Cannot symlink ${sln}.so : $!";
}

