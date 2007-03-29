$DDS_ROOT/MPC/config-legacy
---------------------------

This directory holds MPC config files (usually .mpb) that are needed when
building DDS with older versions of ACE+TAO.  If MPC reports a missing
base project and you are using an old version of ACE+TAO, try adding the
following to your mwc.pl command line:

Unix:
        -include $DDS_ROOT/MPC/config-legacy
Windows:
        -include %DDS_ROOT%\MPC\config-legacy
