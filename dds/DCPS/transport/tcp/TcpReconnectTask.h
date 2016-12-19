/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_TCPRECONNECTTASK_H
#define OPENDDS_TCPRECONNECTTASK_H

#include /**/ "ace/pre.h"

#include "dds/DCPS/transport/framework/QueueTaskBase_T.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class TcpConnection;

enum ReconnectOpType {
  DO_RECONNECT
};

/**
 * @class TcpReconnectTask
 *
 * @brief Active Object managing a queue of reconnecting request.
 *
 *  This task handles request to reconnect to the remotes to avoid the
 *  the caller threads (thread to send or reactor thread) block on reconnecting.
 *  This reconnect task has lifetime as TcpConnection object. One reconnect
 *  task just dedicates to a single connection.
 */
class TcpReconnectTask : public QueueTaskBase <ReconnectOpType> {
public:
  TcpReconnectTask(TcpConnection* con);

  virtual ~TcpReconnectTask();

  /// Handle reconnect requests.
  virtual void execute(ReconnectOpType& op);

private:

  /// The connection that needs be re-established.
  TcpConnection* connection_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#include /**/ "ace/post.h"

#endif /* OPENDDS_TCPRECONNECTTASK_H */
