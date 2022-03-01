eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

use strict;
use warnings;

use File::Basename qw(dirname basename);
use File::Copy qw(copy);
use File::Path qw(make_path);
use Module::Load::Conditional qw(can_load);
use FindBin;

BEGIN {
  require lib;
  if (exists($ENV{ACE_ROOT}) && -d "$ENV{ACE_ROOT}/bin/PerlACE") {
    lib->import("$ENV{ACE_ROOT}/bin");
  }
  elsif (exists($ENV{ACE_SOURCE_ROOT}) && -d "$ENV{ACE_SOURCE_ROOT}/bin/PerlACE") {
    lib->import("$ENV{ACE_SOURCE_ROOT}/bin");
  }
  if (!can_load(modules => {'PerlACE::Process' => undef})) {
    die("ERROR: Couldn't load PerlACE::Process\n" .
      "Set ACE_ROOT or ACE_SOURCE_ROOT to the ACE source or add PerlACE to PERL5LIB");
  }
}
use lib "$FindBin::RealBin/../../bin";
use PerlDDS::Process;

die("ERROR: please pass the install prefix") unless (scalar(@ARGV) == 1);
my $dest_base = "$ARGV[0]/bin";

sub copy_pms_from_same_dir_as {
  my $src_example = shift;

  my $src_dir = dirname($INC{$src_example});
  my $dest_dir = "$dest_base/" . basename($src_dir);
  print("Copy $src_dir to\n  $dest_dir\n");

  my $copied_at_least_one_file = 0;

  opendir(my $dh, $src_dir) or die("ERROR: Couldn't opendir $src_dir: $!");
  foreach my $filename (readdir($dh)) {
    next if ($filename !~ /\.pm$/);
    print("    $filename\n");
    make_path($dest_dir);
    my $src_file = "$src_dir/$filename";
    my $dest_file = "$dest_dir/$filename";
    copy($src_file, $dest_file) or die("ERROR: copy failed: $!");
    $copied_at_least_one_file = 1;
  }
  closedir($dh);

  die("ERROR: No files found to copy!") unless ($copied_at_least_one_file);
}

copy_pms_from_same_dir_as('PerlACE/Process.pm');
copy_pms_from_same_dir_as('PerlDDS/Process.pm');
