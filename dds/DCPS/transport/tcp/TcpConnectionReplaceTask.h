/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_TCPCONNECTIONREPLACETASK_H
#define OPENDDS_TCPCONNECTIONREPLACETASK_H

#include /**/ "ace/pre.h"

#include "TcpConnection_rch.h"
#include "dds/DCPS/transport/framework/QueueTaskBase_T.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class TcpTransport;

/**
 * @class TcpConnectionReplaceTask
 *
 * @brief Active Object managing a queue of connection info objects.
 *
 *  This task is dedicated to check if the incoming connections are re-established
 *  connection from the remote. This would resolve the deadlock problem between the
 *  reactor thread (calling TcpTransport::passive_connection()) and the orb
 *  thread (calling TcpTransport::make_passive_connection()). The reactor
 *  thread will enqueue the new connection to this task and let this task dequeue
 *  and check the connection. This task handles all connections associated with
 *  a TransportImpl object.
 */
class TcpConnectionReplaceTask : public QueueTaskBase <TcpConnection_rch> {
public:
  TcpConnectionReplaceTask(TcpTransport* trans);

  virtual ~TcpConnectionReplaceTask();

  /// Handle the request.
  virtual void execute(TcpConnection_rch& con);

private:

  TcpTransport* trans_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#include /**/ "ace/post.h"

#endif /* OPENDDS_TCPCONNECTIONREPLACETASK_H */
