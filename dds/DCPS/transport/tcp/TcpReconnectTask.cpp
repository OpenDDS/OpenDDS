/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "dds/DCPS/transport/tcp/TcpReconnectTask.h"
#include "dds/DCPS/transport/tcp/TcpConnection.h"
#include "dds/DCPS/transport/framework/EntryExit.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

TcpReconnectTask::TcpReconnectTask() : connection_(), mutex_(), cv_(mutex_)
{
  DBG_ENTRY_LVL("TcpReconnectTask", "TcpReconnectTask", 6);
  if (open()) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: Reconnect task failed to open : %p\n"),
               ACE_TEXT("open")));
  }
}

TcpReconnectTask::~TcpReconnectTask()
{
  DBG_ENTRY_LVL("TcpReconnectTask", "~TcpReconnectTask", 6);
}

bool TcpReconnectTask::reconnect(TcpConnection_rch connection)
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, mutex_, 0);
  if (!connection_) {
    connection_ = connection;
    activate(1);
  }
  return false;
}

void TcpReconnectTask::wait_complete()
{
  ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
  while (connection_) {
    cv_.wait();
  }
}

int TcpReconnectTask::svc()
{
  DBG_ENTRY_LVL("TcpReconnectTask", "svc", 6);

  TcpConnection_rch connection;
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, mutex_, 0);
    connection = connection_;
  }

  if (connection && connection->reconnect() == -1) {
    connection->tear_link();
  }

  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, mutex_, 0);
    connection_.reset();
    cv_.broadcast();
  }

  return 0;
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
