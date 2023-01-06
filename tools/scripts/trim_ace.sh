#!/bin/sh

# ******************************************************************
#      Author: Chad Elliott
#        Date: 1/5/2023
# Description: Strip down ACE and TAO to only the files necessary
#              for OpenDDS, add CMake support, and commit everything
#              to a branch.
# ******************************************************************

# ******************************************************************
# Data Section
# ******************************************************************

START=`pwd`
ORIGIN=`dirname $0`
VERSION="ACE/ace/Version.h"
DRY_RUN=

# ******************************************************************
# Main Section
# ******************************************************************

ROOT=
usage=0
while [ $# -ne 0 ]; do
  arg="$1"
  shift
  case $arg in
    --dry-run)
      DRY_RUN=echo
      ;;
    -*)
      usage=1
      ;;
    *)
      if [ -z "$ROOT" ]; then
        ROOT=$arg
      else
        ## Either an extra parameter or unknown option.  Force the usage
        ## message.
        usage=1
        echo "Unrecognized option: $arg"
      fi
      ;;
  esac
done

if [ -z "$ROOT" -o $usage -eq 1 ]; then
  echo "Usage: `basename $0` [--dry-run] <ACE_TAO checkout>"
  echo ""
  echo "This script will strip ACE and TAO of all non-essential files.  It will then,"
  echo "edit the ACE version to distinguish it from the main stream.  And finally, will"
  echo "commit those changes to a branch.  The changes will not be pushed automatically."
  exit 0
fi

if [ ! -d $ROOT ]; then
  echo "$ROOT must be a directory."
  exit 1
fi

## Remove ACE files and directories that are not necessary for OpenDDS
echo "Stripping ACE..."
cd $ROOT/ACE
remove=`ls -1 | grep -v ace`
if [ ! -z "$remove" ]; then
  $DRY_RUN git rm -rq $remove
fi

cd ace
$DRY_RUN git rm -rq Compression ETCL FlReactor FoxReactor Monitor_Control QoS QtReactor SSL TkReactor XtReactor

$DRY_RUN git rm -q ace.mpc ace.mwc ACE.pc.in ace.rc ACE_crc_ccitt.cpp \
ace_for_tao.mpc ACE_FOR_TAO.pc.in ace_message_table.bin Activation_Queue.cpp \
Activation_Queue.h Activation_Queue.inl Active_Map_Manager.cpp \
Active_Map_Manager.h Active_Map_Manager.inl Asynch_Acceptor.cpp \
Asynch_Acceptor.h Asynch_Connector.cpp Asynch_Connector.h ATM_Acceptor.cpp \
ATM_Acceptor.h ATM_Acceptor.inl ATM_Addr.cpp ATM_Addr.h ATM_Addr.inl \
ATM_Connector.cpp ATM_Connector.h ATM_Connector.inl ATM_Params.cpp \
ATM_Params.h ATM_Params.inl ATM_QoS.cpp ATM_QoS.h ATM_QoS.inl ATM_Stream.cpp \
ATM_Stream.h ATM_Stream.inl Barrier.cpp Barrier.h Capabilities.cpp \
Capabilities.h Capabilities.inl CE_Screen_Output.cpp CE_Screen_Output.h \
Codecs.cpp Codecs.h codecs.mpb Codeset_IBM1047.cpp Codeset_IBM1047.h \
Codeset_Registry.cpp Codeset_Registry.h Codeset_Registry.inl \
Codeset_Registry_db.cpp Connection_Recycling_Strategy.h \
Connection_Recycling_Strategy.cpp Date_Time.cpp Date_Time.h Date_Time.inl \
DEV.cpp DEV.h DEV.inl DEV_Addr.cpp DEV_Addr.h DEV_Addr.inl DEV_Connector.cpp \
DEV_Connector.h DEV_Connector.inl DEV_IO.cpp DEV_IO.h DEV_IO.inl \
Dirent_Selector.cpp Dirent_Selector.h Dirent_Selector.inl Dump.cpp Dump.h \
Dynamic.cpp FIFO.cpp FIFO.h FIFO.inl FIFO_Recv.cpp FIFO_Recv.h FIFO_Recv.inl \
FIFO_Recv_Msg.cpp FIFO_Recv_Msg.h FIFO_Recv_Msg.inl FIFO_Send.cpp FIFO_Send.h \
FIFO_Send.inl FIFO_Send_Msg.cpp FIFO_Send_Msg.h FIFO_Send_Msg.inl FILE.cpp \
FILE.h FILE.inl FILE_Addr.cpp FILE_Addr.h FILE_Addr.inl FILE_Connector.cpp \
FILE_Connector.h FILE_Connector.inl FILE_IO.cpp FILE_IO.h FILE_IO.inl \
File_Lock.cpp File_Lock.h File_Lock.inl Filecache.cpp Filecache.h \
filecache.mpb Future.cpp Future.h Future_Set.cpp Future_Set.h Handle_Ops.cpp \
Handle_Ops.h Hash_Multi_Map_Manager_T.h Hash_Multi_Map_Manager_T.inl \
Hash_Multi_Map_Manager_T.cpp ICMP_Socket.cpp ICMP_Socket.h IO_SAP.cpp \
IO_SAP.h IO_SAP.inl IOStream.cpp IOStream.h Local_Name_Space.cpp \
Local_Name_Space.h Local_Name_Space_T.cpp Local_Name_Space_T.h \
Local_Tokens.cpp Local_Tokens.h Local_Tokens.inl LOCK_SOCK_Acceptor.cpp \
LOCK_SOCK_Acceptor.h Log_Msg_Android_Logcat.cpp Log_Msg_Android_Logcat.h \
LSOCK.cpp LSOCK.h LSOCK.inl LSOCK_Acceptor.cpp LSOCK_Acceptor.h \
LSOCK_CODgram.cpp LSOCK_CODgram.h LSOCK_CODgram.inl LSOCK_Connector.cpp \
LSOCK_Connector.h LSOCK_Connector.inl LSOCK_Dgram.cpp LSOCK_Dgram.h \
LSOCK_Dgram.inl LSOCK_Stream.cpp LSOCK_Stream.h LSOCK_Stream.inl \
MEM_Acceptor.cpp MEM_Acceptor.h MEM_Acceptor.inl MEM_Addr.cpp MEM_Addr.h \
MEM_Addr.inl MEM_Connector.cpp MEM_Connector.h MEM_Connector.inl MEM_IO.cpp \
MEM_IO.h MEM_IO.inl MEM_SAP.cpp MEM_SAP.h MEM_SAP.inl MEM_Stream.cpp \
MEM_Stream.h MEM_Stream.inl Method_Request.cpp Method_Request.h \
Metrics_Cache_T.cpp Metrics_Cache_T.h Metrics_Cache_T.inl Monitor_Admin.cpp \
Monitor_Admin.h Monitor_Admin_Manager.cpp Monitor_Admin_Manager.h \
Monitor_Base.cpp Monitor_Base.h Monitor_Base.inl Monitor_Control_Action.cpp \
Monitor_Control_Action.h Monitor_Control_Types.cpp Monitor_Control_Types.h \
Monitor_Point_Registry.cpp Monitor_Point_Registry.h Monitor_Size.cpp \
Monitor_Size.h MQX_Filesystem.cpp MQX_Filesystem.h Msg_WFMO_Reactor.cpp \
Msg_WFMO_Reactor.h Msg_WFMO_Reactor.inl Multihomed_INET_Addr.cpp \
Multihomed_INET_Addr.h Multihomed_INET_Addr.inl Name_Proxy.cpp Name_Proxy.h \
Name_Request_Reply.cpp Name_Request_Reply.h Name_Space.cpp Name_Space.h \
Naming_Context.cpp Naming_Context.h Naming_Context.inl Netlink_Addr.cpp \
Netlink_Addr.h Netlink_Addr.inl NT_Service.cpp NT_Service.h NT_Service.inl \
OS_NS_devctl.cpp OS_NS_devctl.h OS_NS_devctl.inl OS_TLI.cpp other.mpb \
Pagefile_Memory_Pool.cpp Pagefile_Memory_Pool.h Pagefile_Memory_Pool.inl \
PI_Malloc.cpp PI_Malloc.h PI_Malloc.inl Ping_Socket.cpp Ping_Socket.h \
Ping_Socket.inl POSIX_Asynch_IO.cpp POSIX_Asynch_IO.h POSIX_CB_Proactor.cpp \
POSIX_CB_Proactor.h POSIX_Proactor.cpp POSIX_Proactor.h POSIX_Proactor.inl \
Priority_Reactor.cpp Priority_Reactor.h Process_Manager.cpp Process_Manager.h \
Process_Manager.inl Process_Semaphore.cpp Process_Semaphore.h \
Process_Semaphore.inl Profile_Timer.cpp Profile_Timer.h Profile_Timer.inl \
Reactor_Notification_Strategy.h Reactor_Notification_Strategy.inl \
Reactor_Notification_Strategy.cpp Read_Buffer.cpp Read_Buffer.h \
Read_Buffer.inl README Recyclable.cpp Recyclable.h Recyclable.inl \
Registry.cpp Registry.h Registry_Name_Space.cpp Registry_Name_Space.h \
Remote_Name_Space.cpp Remote_Name_Space.h Remote_Tokens.cpp Remote_Tokens.h \
Remote_Tokens.inl RW_Process_Mutex.cpp RW_Process_Mutex.h \
RW_Process_Mutex.inl Sample_History.cpp Sample_History.h Sample_History.inl \
Sbrk_Memory_Pool.cpp Sbrk_Memory_Pool.h Shared_Memory.cpp Shared_Memory.h \
Shared_Memory_MM.cpp Shared_Memory_MM.h Shared_Memory_MM.inl \
Shared_Memory_Pool.cpp Shared_Memory_Pool.h Shared_Memory_SV.cpp \
Shared_Memory_SV.h Shared_Memory_SV.inl SOCK_CODgram.cpp SOCK_CODgram.h \
SOCK_CODgram.inl SOCK_Dgram_Bcast.cpp SOCK_Dgram_Bcast.h SOCK_Dgram_Bcast.inl \
SOCK_Netlink.cpp SOCK_Netlink.h SOCK_Netlink.inl SOCK_SEQPACK_Acceptor.cpp \
SOCK_SEQPACK_Acceptor.h SOCK_SEQPACK_Acceptor.inl SOCK_SEQPACK_Association.h \
SOCK_SEQPACK_Association.inl SOCK_SEQPACK_Association.cpp \
SOCK_SEQPACK_Connector.cpp SOCK_SEQPACK_Connector.h \
SOCK_SEQPACK_Connector.inl SPIPE.cpp SPIPE.h SPIPE.inl SPIPE_Acceptor.cpp \
SPIPE_Acceptor.h SPIPE_Addr.cpp SPIPE_Addr.h SPIPE_Addr.inl \
SPIPE_Connector.cpp SPIPE_Connector.h SPIPE_Connector.inl SPIPE_Stream.cpp \
SPIPE_Stream.h SPIPE_Stream.inl SUN_Proactor.cpp SUN_Proactor.h \
SV_Message.cpp SV_Message.h SV_Message.inl SV_Message_Queue.cpp \
SV_Message_Queue.h SV_Message_Queue.inl SV_Shared_Memory.cpp \
SV_Shared_Memory.h SV_Shared_Memory.inl Svc_Conf.y svcconf.mpb svcconfgen.mpc \
Synch_Options.cpp Synch_Options.h System_Time.cpp System_Time.h \
Test_and_Set.cpp Test_and_Set.h Thread_Semaphore.cpp Thread_Semaphore.h \
Thread_Semaphore.inl Throughput_Stats.cpp Throughput_Stats.h Timeprobe.cpp \
TLI.cpp TLI.h TLI.inl TLI_Acceptor.cpp TLI_Acceptor.h TLI_Connector.cpp \
TLI_Connector.h TLI_Connector.inl TLI_Stream.cpp TLI_Stream.h TLI_Stream.inl \
token.mpb Token_Collection.cpp Token_Collection.h Token_Collection.inl \
Token_Invariants.cpp Token_Invariants.h Token_Manager.cpp Token_Manager.h \
Token_Manager.inl Token_Request_Reply.cpp Token_Request_Reply.h \
Token_Request_Reply.inl TTY_IO.cpp TTY_IO.h UNIX_Addr.cpp UNIX_Addr.h \
UNIX_Addr.inl UPIPE_Acceptor.cpp UPIPE_Acceptor.h UPIPE_Acceptor.inl \
UPIPE_Connector.cpp UPIPE_Connector.h UPIPE_Connector.inl UPIPE_Stream.cpp \
UPIPE_Stream.h UPIPE_Stream.inl UUID.cpp UUID.h UUID.inl uuid.mpb \
XTI_ATM_Mcast.cpp XTI_ATM_Mcast.h XTI_ATM_Mcast.inl

## Remove TAO files and directories that are not necessary for OpenDDS
echo "Stripping TAO..."
cd $ROOT/TAO
remove=`ls -1 | grep -v TAO_IDL | grep -v '^tao$'`
if [ ! -z "$remove" ]; then
  $DRY_RUN git rm -rq $remove
fi

$DRY_RUN git rm -rq TAO_IDL/be TAO_IDL/be_include TAO_IDL/*.m?c

cd tao
remove=`ls -1 | grep -v idl_features.h`
if [ ! -z "$remove" ]; then
  $DRY_RUN git rm -rq $remove
fi

## Put the CMakeLists.* files into place
echo "Adding CMake support..."
add_cmake="CMakeLists.txt ACE/ace/CMakeLists.txt ACE/cmake/ACE_for_OpenDDSConfig.cmake TAO/TAO_IDL/CMakeLists.txt"
$DRY_RUN cd $START
$DRY_RUN cd $ORIGIN/ace_cmake
if [ -z "$DRY_RUN" ]; then
  tar cf - $add_cmake | (cd $ROOT; tar xf -)
else
  $DRY_RUN tar cf - $add_cmake ...
fi

## Update ACE version header file
echo "Editing the ACE version..."
$DRY_RUN sed -i -E 's/(ACE_VERSION "[0-9\.]+)"/\1 for OpenDDS"/' $ROOT/$VERSION

## Commit the changes
echo "Committing changes..."
cd $ROOT
branch=`grep 'ACE_VERSION ' $VERSION | sed -E 's/.*"(.*)"/\1/' | sed 's/ /_/g'`
$DRY_RUN git checkout -b $branch
$DRY_RUN git add $add_cmake $VERSION
$DRY_RUN git commit -m 'Removed anything that was not necessary for OpenDDS'
echo '** Changes have been committed to this checkout, but not pushed. **'
