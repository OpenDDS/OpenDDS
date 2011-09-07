/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DCPS_RTPSUDPDATALINK_H
#define DCPS_RTPSUDPDATALINK_H

#include "Rtps_Udp_Export.h"

#include "RtpsUdpSendStrategy.h"
#include "RtpsUdpSendStrategy_rch.h"
#include "RtpsUdpReceiveStrategy.h"
#include "RtpsUdpReceiveStrategy_rch.h"

#include "ace/Basic_Types.h"
#include "ace/SOCK_Dgram.h"

#include "dds/DCPS/transport/framework/DataLink.h"
#include "dds/DCPS/transport/framework/TransportReactorTask.h"

namespace OpenDDS {
namespace DCPS {

class RtpsUdpInst;
class RtpsUdpTransport;
class ReceivedDataSample;

class OpenDDS_Rtps_Udp_Export RtpsUdpDataLink : public DataLink {
public:
  RtpsUdpDataLink(RtpsUdpTransport* transport, bool active);

  void configure(RtpsUdpInst* config,
                 TransportReactorTask* reactor_task);

  void send_strategy(RtpsUdpSendStrategy* send_strategy);
  void receive_strategy(RtpsUdpReceiveStrategy* recv_strategy);

  bool active() const;

  RtpsUdpInst* config();
  TransportReactorTask* reactor_task();

  ACE_Reactor* get_reactor();

  ACE_INET_Addr& remote_address();

  ACE_SOCK_Dgram& socket();

  bool open(const ACE_INET_Addr& remote_address);

  void control_received(ReceivedDataSample& sample,
                        const ACE_INET_Addr& remote_address);

protected:
  bool active_;

  RtpsUdpInst* config_;
  TransportReactorTask* reactor_task_;

  RtpsUdpSendStrategy_rch send_strategy_;
  RtpsUdpReceiveStrategy_rch recv_strategy_;

  virtual void stop_i();

private:
  ACE_INET_Addr remote_address_;

  ACE_SOCK_Dgram socket_;
};

} // namespace DCPS
} // namespace OpenDDS

#ifdef __ACE_INLINE__
# include "RtpsUdpDataLink.inl"
#endif  /* __ACE_INLINE__ */

#endif  /* DCPS_RTPSUDPDATALINK_H */
