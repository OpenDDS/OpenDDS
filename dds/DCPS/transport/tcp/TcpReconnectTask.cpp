/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "Tcp_pch.h"
#include "dds/DCPS/transport/tcp/TcpReconnectTask.h"
#include "dds/DCPS/transport/tcp/TcpConnection.h"
#include "dds/DCPS/transport/tcp/TcpTransport.h"
#include "dds/DCPS/transport/framework/EntryExit.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

TcpReconnectTask::TcpReconnectTask() : connection_(), mutex_(), cv_(mutex_), shutdown_(false), id_(ACE_OS::NULL_thread)
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
  shutdown();
  wait();
}

void TcpReconnectTask::reconnect(TcpConnection_rch connection)
{
  DBG_ENTRY_LVL("TcpReconnectTask", "reconnect", 6);
  ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
  if (!shutdown_ && !connection_) {
    connection_ = connection;

    // We want to set the number of threads (1)  and capture the thread id, but
    // unfortunately this means we have to manually specify most of the defaults
    activate(THR_NEW_LWP | THR_JOINABLE | THR_INHERIT_SCHED, //flags
             1, //n_threads
             0, //force_active
             ACE_DEFAULT_THREAD_PRIORITY, //priority
             -1, //grp_id
             0, //task
             0, //thread_handles[]
             0, //stack[]
             0, //stack_size[]
             &id_, //thread_ids[]
             0); //thr_name
  }
}

bool TcpReconnectTask::active() {
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, mutex_, false);
  return connection_;
}

void TcpReconnectTask::shutdown() {
  DBG_ENTRY_LVL("TcpReconnectTask", "shutdown", 6);
  ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
  shutdown_ = true;
}

void TcpReconnectTask::wait_complete()
{
  DBG_ENTRY_LVL("TcpReconnectTask", "wait_complete", 6);
  ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
  while (connection_) {
    cv_.wait();
  }
}

int TcpReconnectTask::svc()
{
  DBG_ENTRY_LVL("TcpReconnectTask", "svc", 6);
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, mutex_, 0);

  // The order here matters for destruction if we're the last reference holders by the end
  TcpTransport_rch impl(connection_->impl());
  TcpConnection_rch connection(connection_);

  ACE_Reverse_Lock<ACE_Thread_Mutex> rev_lock(mutex_);

  if (connection) {
    ACE_GUARD_RETURN(ACE_Reverse_Lock<ACE_Thread_Mutex>, rev_guard, rev_lock, 0);
    if (connection->reconnect() == -1) {
      connection->tear_link();
    }
  }

  connection_.reset();
  cv_.broadcast();

  // So we're not holding the mutex if/when we're the last one holding reference to TcpConnection
  g.release();

  return 0;
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
