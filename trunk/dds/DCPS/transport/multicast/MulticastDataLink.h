/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DCPS_MULTICASTDATALINK_H
#define DCPS_MULTICASTDATALINK_H

#include "Multicast_Export.h"

#include "MulticastTransport_rch.h"
#include "MulticastConfiguration.h"
#include "MulticastConfiguration_rch.h"
#include "MulticastSendStrategy.h"
#include "MulticastSendStrategy_rch.h"
#include "MulticastReceiveStrategy.h"
#include "MulticastReceiveStrategy_rch.h"

#include "ace/Basic_Types.h"
#include "ace/SOCK_Dgram_Mcast.h"

#include "dds/DCPS/transport/framework/DataLink.h"
#include "dds/DCPS/transport/framework/TransportReactorTask.h"
#include "dds/DCPS/transport/framework/TransportReactorTask_rch.h"

namespace OpenDDS {
namespace DCPS {

typedef ACE_INT32 MulticastPeer;
typedef ACE_INT16 MulticastSequence;

class OpenDDS_Multicast_Export MulticastDataLink
  : public DataLink {
public:
  MulticastDataLink(MulticastTransport* transport,
                    MulticastPeer local_peer,
                    MulticastPeer remote_peer,
                    bool active);
  virtual ~MulticastDataLink();

  void configure(MulticastConfiguration* config,
                 TransportReactorTask* reactor_task);

  virtual void send_strategy(MulticastSendStrategy* send_strategy);
  virtual void receive_strategy(MulticastReceiveStrategy* recv_strategy);

  MulticastPeer local_peer() const;
  MulticastPeer remote_peer() const;

  bool active() const;

  MulticastConfiguration* config();
  TransportReactorTask* reactor_task();

  ACE_Reactor* get_reactor();

  ACE_SOCK_Dgram_Mcast& socket();
  bool join(const ACE_INET_Addr& group_address);

  virtual bool acked() = 0;

  virtual bool header_received(const TransportHeader& header) = 0;
  virtual void sample_received(ReceivedDataSample& sample) = 0;

protected:
  MulticastTransport_rch transport_;

  MulticastPeer local_peer_;
  MulticastPeer remote_peer_;

  bool active_;

  MulticastConfiguration_rch config_;
  TransportReactorTask_rch reactor_task_;

  MulticastSendStrategy_rch send_strategy_;
  MulticastReceiveStrategy_rch recv_strategy_;

  virtual void stop_i();

private:
  ACE_SOCK_Dgram_Mcast socket_;
};

} // namespace DCPS
} // namespace OpenDDS

#ifdef __ACE_INLINE__
# include "MulticastDataLink.inl"
#endif  /* __ACE_INLINE__ */

#endif  /* DCPS_MULTICASTDATALINK_H */
