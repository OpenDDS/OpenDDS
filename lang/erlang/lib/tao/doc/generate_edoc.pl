#
# $Id$
#

eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
  & eval 'exec perl -S $0 $argv:q'
  if 0;

use strict;
use warnings;

use Env qw(DDS_ROOT);
use lib qq($DDS_ROOT/lib/perl);

use Erlang::EDoc;

my $edoc = new Erlang::EDoc;
$edoc->files('../src',
  '{dir, "html"}',
  '{overview, "overview.edoc"}',
  '{title, "Overview (tao)"}',
);
exit $!;
