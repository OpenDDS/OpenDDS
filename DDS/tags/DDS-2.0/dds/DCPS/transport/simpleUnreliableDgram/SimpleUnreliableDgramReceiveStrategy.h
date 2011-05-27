/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_SIMPLEUNRELIABLEDGRAMRECEIVESTRATEGY_H
#define OPENDDS_DCPS_SIMPLEUNRELIABLEDGRAMRECEIVESTRATEGY_H

#include "SimpleUnreliableDgram_export.h"
#include "SimpleUnreliableDgramSocket_rch.h"
#include "SimpleUnreliableDgramTransport_rch.h"
#include "dds/DCPS/transport/framework/TransportReceiveStrategy.h"
#include "dds/DCPS/transport/framework/TransportReactorTask_rch.h"

namespace OpenDDS {
namespace DCPS {

class SimpleUnreliableDgram_Export SimpleUnreliableDgramReceiveStrategy
  : public TransportReceiveStrategy {
public:

  SimpleUnreliableDgramReceiveStrategy(SimpleUnreliableDgramTransport* transport,
                                       SimpleUnreliableDgramSocket* socket,
                                       TransportReactorTask* task);
  virtual ~SimpleUnreliableDgramReceiveStrategy();

protected:

  virtual ssize_t receive_bytes(iovec          iov[],
                                int            n,
                                ACE_INET_Addr& remote_address);

  virtual void deliver_sample(ReceivedDataSample&  sample,
                              const ACE_INET_Addr& remote_address);

  virtual int start_i();
  virtual void stop_i();

private:

  SimpleUnreliableDgramTransport_rch   transport_;
  SimpleUnreliableDgramSocket_rch  socket_;
  TransportReactorTask_rch task_;
};

} // namespace DCPS
} // namespace OpenDDS

#if defined (__ACE_INLINE__)
#include "SimpleUnreliableDgramReceiveStrategy.inl"
#endif /* __ACE_INLINE__ */

#endif  /* OPENDDS_DCPS_SIMPLEUNRELIABLEDGRAMRECEIVESTRATEGY_H */
