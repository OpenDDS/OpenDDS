/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DCPS_MULTICASTDATALINK_H
#define DCPS_MULTICASTDATALINK_H

#include "Multicast_Export.h"

#include "MulticastTransport.h"
#include "MulticastTransport_rch.h"
#include "MulticastConfiguration.h"
#include "MulticastConfiguration_rch.h"
#include "MulticastSendStrategy_rch.h"
#include "MulticastReceiveStrategy_rch.h"

#include "ace/Basic_Types.h"
#include "ace/SOCK_Dgram_Mcast.h"

#include "dds/DCPS/transport/framework/DataLink.h"
#include "dds/DCPS/transport/framework/TransportReactorTask.h"
#include "dds/DCPS/transport/framework/TransportReactorTask_rch.h"

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Multicast_Export MulticastDataLink
  : public DataLink {
public:
  MulticastDataLink(MulticastTransport* transport,
                    ACE_INT32 local_peer,
                    ACE_INT32 remote_peer);
  
  virtual ~MulticastDataLink();

  void configure(MulticastConfiguration* config,
                 TransportReactorTask* reactor_task);

  void send_strategy(MulticastSendStrategy* send_strategy);
  void receive_strategy(MulticastReceiveStrategy* recv_strategy);

  ACE_INT32 local_peer() const;
  ACE_INT32 remote_peer() const;

  MulticastConfiguration* config();
  TransportReactorTask* reactor_task();

  ACE_Reactor* get_reactor();

  ACE_SOCK_Dgram_Mcast& socket();

  bool join(const ACE_INET_Addr& group_address, bool active);
  void leave();

  virtual void sample_received(ReceivedDataSample& sample) = 0;
  virtual bool acked() = 0;

protected:
  MulticastTransport_rch transport_;

  ACE_INT32 local_peer_;
  ACE_INT32 remote_peer_;

  MulticastConfiguration_rch config_;
  TransportReactorTask_rch reactor_task_;

  virtual bool join_i(const ACE_INET_Addr& group_address, bool active) = 0;
  virtual void leave_i() = 0;

  virtual void stop_i();

private:
  MulticastSendStrategy_rch send_strategy_;
  MulticastReceiveStrategy_rch recv_strategy_;

  ACE_SOCK_Dgram_Mcast socket_;
};

} // namespace DCPS
} // namespace OpenDDS

#ifdef __ACE_INLINE__
# include "MulticastDataLink.inl"
#endif  /* __ACE_INLINE__ */

#endif  /* DCPS_MULTICASTDATALINK_H */
