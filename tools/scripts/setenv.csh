#
#

#
# Environment variables needed to build ACE+TAO and OpenDDS.
#
setenv MPC_ROOT /path/to/MPC
setenv ACE_ROOT /path/to/ACE
setenv TAO_ROOT /path/to/TAO
setenv DDS_ROOT /path/to/DDS

#
# NO CHANGES BELOW THIS LINE
#
setenv LD_LIBRARY_PATH $ACE_ROOT/lib:$DDS_ROOT/lib
setenv PATH $ACE_ROOT/bin:$DDS_ROOT/bin:$PATH

echo "MPC_ROOT  is $MPC_ROOT"
echo "ACE_ROOT  is $ACE_ROOT"
echo "TAO_ROOT  is $TAO_ROOT"
echo "DDS_ROOT  is $DDS_ROOT"
echo
echo "(pwd: `pwd`)"
