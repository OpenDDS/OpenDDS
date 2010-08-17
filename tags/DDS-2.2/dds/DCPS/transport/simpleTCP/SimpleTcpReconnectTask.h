/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_SIMPLETCPRECONNECTTASK_H
#define OPENDDS_DCPS_SIMPLETCPRECONNECTTASK_H

#include /**/ "ace/pre.h"

#include "dds/DCPS/transport/framework/QueueTaskBase_T.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

namespace OpenDDS {
namespace DCPS {

class SimpleTcpConnection;

enum ReconnectOpType {
  DO_RECONNECT
};

/**
 * @class SimpleTcpReconnectTask
 *
 * @brief Active Object managing a queue of reconnecting request.
 *
 *  This task handles request to reconnect to the remotes to avoid the
 *  the caller threads (thread to send or reactor thread) block on reconnecting.
 *  This reconnect task has lifetime as SimpleTcpConnection object. One reconnect
 *  task just dedicates to a single connection.
 */
class SimpleTcpReconnectTask : public QueueTaskBase <ReconnectOpType> {
public:

  /// Constructor.
  SimpleTcpReconnectTask(SimpleTcpConnection* con);

  /// Virtual Destructor.
  virtual ~SimpleTcpReconnectTask();

  /// Handle reconnect requests.
  virtual void execute(ReconnectOpType& op);

private:

  /// The connection that needs be re-established.
  SimpleTcpConnection* connection_;
};

} // namespace DCPS
} // namespace OpenDDS

#include /**/ "ace/post.h"

#endif /* OPENDDS_DCPS_SIMPLETCPRECONNECTTASK_H */
