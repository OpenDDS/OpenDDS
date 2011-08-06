/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_TCPSYNCHRESOURCE_H
#define OPENDDS_TCPSYNCHRESOURCE_H

#include "TcpConnection_rch.h"
#include "TcpConnection.h"
#include "dds/DCPS/transport/framework/ThreadSynchResource.h"
#include "ace/Handle_Set.h"
#include "ace/Time_Value.h"

namespace OpenDDS {
namespace DCPS {

class TcpSynchResource : public ThreadSynchResource {
public:

  TcpSynchResource(TcpConnection* connection,
                         const int& max_output_pause_period_ms);
  virtual ~TcpSynchResource();

  virtual void notify_lost_on_backpressure_timeout();

private:

  TcpConnection_rch connection_;
};

} // namespace DCPS
} // namespace OpenDDS

#endif  /* OPENDDS_TCPSYNCHRESOURCE_H */
