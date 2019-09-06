/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_TCPRECONNECTTASK_H
#define OPENDDS_TCPRECONNECTTASK_H

#include /**/ "ace/pre.h"

#include "dds/DCPS/transport/tcp/TcpConnection_rch.h"
#include "ace/Condition_Thread_Mutex.h"
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
class TcpReconnectTask : public ACE_Task_Base {
public:
  TcpReconnectTask();

  virtual ~TcpReconnectTask();

  bool reconnect(TcpConnection_rch con);
  void wait_complete();

private:

  /// Handle reconnect requests.
  int svc();

  /// The connection that needs be re-established.
  RcHandle<TcpConnection> connection_;
  ACE_Thread_Mutex mutex_;
  ACE_Condition_Thread_Mutex cv_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#include /**/ "ace/post.h"

#endif /* OPENDDS_TCPRECONNECTTASK_H */
