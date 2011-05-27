/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_SIMPLEUNRELIABLEDGRAMSYNCHRESOURCE_H
#define OPENDDS_DCPS_SIMPLEUNRELIABLEDGRAMSYNCHRESOURCE_H

#include "SimpleUnreliableDgram_export.h"
#include "SimpleUnreliableDgramSocket_rch.h"
#include "SimpleUnreliableDgramTransport.h"
#include "SimpleUnreliableDgramTransport_rch.h"
#include "dds/DCPS/transport/framework/ThreadSynchResource.h"
#include "ace/Handle_Set.h"
#include "ace/Time_Value.h"

namespace OpenDDS {
namespace DCPS {

class SimpleUnreliableDgram_Export SimpleUnreliableDgramSynchResource
  : public ThreadSynchResource {
public:

  SimpleUnreliableDgramSynchResource(SimpleUnreliableDgramSocket*  socket,
                                     SimpleUnreliableDgramTransport* transport,
                                     const int& max_output_pause_period_ms);
  virtual ~SimpleUnreliableDgramSynchResource();

  void notify_lost_on_backpressure_timeout();

private:

  SimpleUnreliableDgramSocket_rch socket_;
  SimpleUnreliableDgramTransport_rch transport_;
};

} // namespace DCPS
} // namespace OpenDDS

#endif  /* OPENDDS_DCPS_SIMPLEUNRELIABLEDGRAMSYNCHRESOURCE_H */
