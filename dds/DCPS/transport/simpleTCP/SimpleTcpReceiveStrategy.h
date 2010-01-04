/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_SIMPLETCPRECEIVESTRATEGY_H
#define OPENDDS_DCPS_SIMPLETCPRECEIVESTRATEGY_H

#include "SimpleTcpConnection_rch.h"
#include "SimpleTcpConnection.h"
#include "SimpleTcpDataLink_rch.h"
#include "dds/DCPS/transport/framework/TransportReceiveStrategy.h"
#include "dds/DCPS/transport/framework/TransportReactorTask_rch.h"

namespace OpenDDS {
namespace DCPS {

class SimpleTcpReceiveStrategy : public TransportReceiveStrategy {
public:

  SimpleTcpReceiveStrategy(SimpleTcpDataLink*    link,
                           SimpleTcpConnection*  connection,
                           TransportReactorTask* task);
  virtual ~SimpleTcpReceiveStrategy();

  int reset(SimpleTcpConnection* connection);

  ACE_Reactor* get_reactor();

  bool gracefully_disconnected();

protected:

  virtual ssize_t receive_bytes(iovec iov[],
                                int   n,
                                ACE_INET_Addr& remote_address);

  virtual void deliver_sample(ReceivedDataSample& sample,
                              const ACE_INET_Addr& remote_address);

  virtual int start_i();
  virtual void stop_i();

  // Delegate to the connection object to re-establishment
  // the connection.
  virtual void relink(bool do_suspend = true);

private:

  SimpleTcpDataLink_rch    link_;
  SimpleTcpConnection_rch  connection_;
  TransportReactorTask_rch reactor_task_;
};

} // namespace DCPS
} // namespace OpenDDS

#if defined (__ACE_INLINE__)
#include "SimpleTcpReceiveStrategy.inl"
#endif /* __ACE_INLINE__ */

#endif  /* OPENDDS_DCPS_SIMPLETCPRECEIVESTRATEGY_H */
