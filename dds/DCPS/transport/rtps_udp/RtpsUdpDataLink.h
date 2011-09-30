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
#include "ace/SOCK_Dgram_Mcast.h"

#include "dds/DCPS/transport/framework/DataLink.h"
#include "dds/DCPS/transport/framework/TransportReactorTask.h"
#include "dds/DCPS/transport/framework/TransportReactorTask_rch.h"

#include <map>
#include <set>

namespace OpenDDS {
namespace DCPS {

class RtpsUdpInst;
class RtpsUdpTransport;
class ReceivedDataSample;

class OpenDDS_Rtps_Udp_Export RtpsUdpDataLink : public DataLink {
public:

  RtpsUdpDataLink(RtpsUdpTransport* transport,
                  const GuidPrefix_t& local_prefix);

  void configure(RtpsUdpInst* config, TransportReactorTask* reactor_task);

  void send_strategy(RtpsUdpSendStrategy* send_strategy);
  void receive_strategy(RtpsUdpReceiveStrategy* recv_strategy);

  RtpsUdpInst* config();
  TransportReactorTask* reactor_task();

  ACE_Reactor* get_reactor();

  ACE_SOCK_Dgram& unicast_socket();
  ACE_SOCK_Dgram_Mcast& multicast_socket();

  bool open();

  void control_received(ReceivedDataSample& sample,
                        const ACE_INET_Addr& remote_address);

  const GuidPrefix_t& local_prefix() const { return local_prefix_; }

  void add_locator(const RepoId& remote_id, const ACE_INET_Addr& address);

  void get_locators(const RepoId& local_id,
                    std::set<ACE_INET_Addr>& addrs) const;

private:
  virtual void stop_i();

  virtual TransportQueueElement* customize_queue_element(
    TransportQueueElement* element);

  RtpsUdpInst* config_;
  TransportReactorTask_rch reactor_task_;

  RtpsUdpSendStrategy_rch send_strategy_;
  RtpsUdpReceiveStrategy_rch recv_strategy_;

  GuidPrefix_t local_prefix_;
  std::map<RepoId, ACE_INET_Addr, GUID_tKeyLessThan> locators_;

  ACE_SOCK_Dgram unicast_socket_;
  ACE_SOCK_Dgram_Mcast multicast_socket_;
};

} // namespace DCPS
} // namespace OpenDDS

#ifdef __ACE_INLINE__
# include "RtpsUdpDataLink.inl"
#endif  /* __ACE_INLINE__ */

#endif  /* DCPS_RTPSUDPDATALINK_H */
