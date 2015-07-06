#
#

#
# Environment variables needed to build ACE+TAO and OpenDDS.
#
MPC_ROOT=/path/to/MPC;    export MPC_ROOT
ACE_ROOT=/path/to/ACE;    export ACE_ROOT
TAO_ROOT=/path/to/TAO;    export TAO_ROOT
DDS_ROOT=/path/to/DDS;    export DDS_ROOT

#
# NO CHANGES BELOW THIS LINE
#
LD_LIBRARY_PATH=$ACE_ROOT/lib:$DDS_ROOT/lib;  export LD_LIBRARY_PATH
PATH=$ACE_ROOT/bin:$DDS_ROOT/bin:$PATH;       export PATH

echo "MPC_ROOT  is $MPC_ROOT"
echo "ACE_ROOT  is $ACE_ROOT"
echo "TAO_ROOT  is $TAO_ROOT"
echo "DDS_ROOT  is $DDS_ROOT"
echo
echo "(pwd: `pwd`)"
