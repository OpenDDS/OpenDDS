/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_TCPRECONNECTTASK_H
#define OPENDDS_TCPRECONNECTTASK_H

#include /**/ "ace/pre.h"

#include "dds/DCPS/RcObject.h"
#include "dds/DCPS/transport/tcp/TcpConnection_rch.h"
#include "ace/Condition_Thread_Mutex.h"
#include "ace/Reverse_Lock_T.h"
#include "ace/Task.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

/**
 * @class TcpReconnectTask
 *
 * @brief Object managing a reconnection request.
 *
 *  This task handles a request to reconnect a tcp connection object to avoid the
 *  the caller threads (thread to send or reactor thread) blocking on reconnect.
 *  This reconnect task has the same lifetime as the paired TcpConnection object.
 */
class TcpReconnectTask : public ACE_Task_Base, public RcObject {
public:
  TcpReconnectTask();

  virtual ~TcpReconnectTask();

  void reconnect(TcpConnection_rch con);
  bool active();
  void wait_complete();
  void shutdown();

  template <typename T>
  void wait_complete(T& lockable) {
    ACE_Reverse_Lock<T> rev_lock(lockable);
    while (active()) {
      ACE_GUARD(ACE_Reverse_Lock<T>, rev_guard, rev_lock);
      wait_complete();
    }
  }

  bool is_task_thread() { return ACE_Thread::self() == id_; }

private:

  /// Handle reconnect requests.
  int svc();

  /// The connection that needs be re-established.
  TcpConnection_rch connection_;
  ACE_Thread_Mutex mutex_;
  ACE_Condition_Thread_Mutex cv_;
  bool shutdown_;
  ACE_thread_t id_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#include /**/ "ace/post.h"

#endif /* OPENDDS_TCPRECONNECTTASK_H */
