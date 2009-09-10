/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_SIMPLETCPSYNCHRESOURCE_H
#define OPENDDS_DCPS_SIMPLETCPSYNCHRESOURCE_H

#include "SimpleTcpConnection_rch.h"
#include "SimpleTcpConnection.h"
#include "dds/DCPS/transport/framework/ThreadSynchResource.h"
#include "ace/Handle_Set.h"
#include "ace/Time_Value.h"

namespace OpenDDS {
namespace DCPS {

class SimpleTcpSynchResource : public ThreadSynchResource {
public:

  SimpleTcpSynchResource(SimpleTcpConnection* connection,
                         const int& max_output_pause_period_ms);
  virtual ~SimpleTcpSynchResource();

  virtual void notify_lost_on_backpressure_timeout();

private:

  SimpleTcpConnection_rch connection_;
};

} // namespace DCPS
} // namespace OpenDDS

#endif  /* OPENDDS_DCPS_SIMPLETCPSYNCHRESOURCE_H */
