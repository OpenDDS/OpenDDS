eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# -*- perl -*-

use Env (DDS_ROOT);
use lib "$DDS_ROOT/bin";
use Env (ACE_ROOT);
use lib "$ACE_ROOT/bin";
use PerlDDS::Run_Test;
use FileHandle;
use Cwd;
use File::Copy;
use strict;

my $test = new PerlDDS::TestFramework();
$test->{add_pending_timeout} = 0;
$test->setup_discovery();

my $dcpsrepo_ior = "repo.ior";
my $dcpsrepo2_ior = "repo2.ior";
my $dcpsrepo3_ior = "repo3.ior";

copy($dcpsrepo_ior, $dcpsrepo2_ior);
copy($dcpsrepo_ior, $dcpsrepo3_ior);

my $cfg = new PerlACE::ConfigList->check_config('NO_BUILT_IN_TOPICS')
          ? 'test1_nobits.ini' : 'test1.ini';

$test->process('ConfigFile', 'ConfigFile', "-DCPSConfigFile $cfg -OpenDDSMyConfigKey1 value1");
$test->process('ConfigFile2', 'ConfigFile', "-DCPSSingleConfigFile 0 -DCPSConfigFile test0.ini -DCPSConfigFile $cfg");

$ENV{'OPENDDS_MY_CONFIG_KEY2'} = "value2";

$test->start_process('ConfigFile');
$test->start_process('ConfigFile2');

my $status = $test->finish(5);

unlink $dcpsrepo2_ior;
unlink $dcpsrepo3_ior;

exit $status;
