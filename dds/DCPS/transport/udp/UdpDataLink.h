/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DCPS_UDPDATALINK_H
#define DCPS_UDPDATALINK_H

#include "Udp_Export.h"

#include "UdpTransport.h"
#include "UdpTransport_rch.h"
#include "UdpConfiguration.h"
#include "UdpConfiguration_rch.h"
#include "UdpSendStrategy.h"
#include "UdpSendStrategy_rch.h"
#include "UdpReceiveStrategy.h"
#include "UdpReceiveStrategy_rch.h"

#include "ace/Basic_Types.h"
#include "ace/SOCK_Dgram.h"

#include "dds/DCPS/transport/framework/DataLink.h"
#include "dds/DCPS/transport/framework/TransportReactorTask.h"
#include "dds/DCPS/transport/framework/TransportReactorTask_rch.h"

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Udp_Export UdpDataLink
  : public DataLink {
public:
  UdpDataLink(UdpTransport* transport,
              bool active);

  void configure(UdpConfiguration* config,
                 TransportReactorTask* reactor_task);

  void send_strategy(UdpSendStrategy* send_strategy);
  void receive_strategy(UdpReceiveStrategy* recv_strategy);

  bool active() const;

  UdpConfiguration* config();
  TransportReactorTask* reactor_task();

  ACE_Reactor* get_reactor();

  ACE_INET_Addr& remote_address();

  ACE_SOCK_Dgram& socket();

  bool open(const ACE_INET_Addr& remote_address);

protected:
  UdpTransport_rch transport_;
  bool active_;

  UdpConfiguration_rch config_;
  TransportReactorTask_rch reactor_task_;

  UdpSendStrategy_rch send_strategy_;
  UdpReceiveStrategy_rch recv_strategy_;

  virtual void stop_i();

private:
  ACE_INET_Addr remote_address_;

  ACE_SOCK_Dgram socket_;
};

} // namespace DCPS
} // namespace OpenDDS

#ifdef __ACE_INLINE__
# include "UdpDataLink.inl"
#endif  /* __ACE_INLINE__ */

#endif  /* DCPS_UDPDATALINK_H */
