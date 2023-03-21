eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

use strict;
use warnings;

use Cwd qw/getcwd/;

use FindBin;
use lib "$FindBin::RealBin/..";
use ChangeDir;

sub assert_file_exists {
  my $path = shift();
  if (!-e $path) {
    die("$path does not exist in " . getcwd());
  }
}

{
  my $cd_here = ChangeDir->new($FindBin::RealBin);

  assert_file_exists("ChangeDir.pl");
  {
    my $cd_up = ChangeDir->new('..');
    assert_file_exists("ChangeDir.pm");
  }
  assert_file_exists("ChangeDir.pl");
}
