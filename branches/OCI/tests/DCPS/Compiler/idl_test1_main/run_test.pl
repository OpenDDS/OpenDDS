eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
     & eval 'exec perl -S $0 $argv:q'
     if 0;

# $Id$
# -*- perl -*-

use lib '../../../../../../bin';
use PerlACE::Run_Test;

$status = 0;

PerlACE::add_lib_path('../idl_test1_lib');

$TESTDVR = new PerlACE::Process ("idl_test1");

$status = $TESTDVR->SpawnWaitKill (300);

if ($status != 0) {
    print STDERR "ERROR: idl_test1 returned $status\n";
    $status = 1;
}


exit $status;
