/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

namespace OpenDDS {
namespace DCPS {

ACE_INLINE void
RtpsUdpDataLink::configure(RtpsUdpInst* config,
                           TransportReactorTask* reactor_task)
{
  config_ = config;
  reactor_task_ = TransportReactorTask_rch(reactor_task, false);
}

ACE_INLINE void
RtpsUdpDataLink::send_strategy(RtpsUdpSendStrategy* send_strategy)
{
  send_strategy_ = send_strategy;
}

ACE_INLINE void
RtpsUdpDataLink::receive_strategy(RtpsUdpReceiveStrategy* recv_strategy)
{
  recv_strategy_ = recv_strategy;
}

ACE_INLINE RtpsUdpInst*
RtpsUdpDataLink::config()
{
  return config_;
}

ACE_INLINE ACE_Reactor*
RtpsUdpDataLink::get_reactor()
{
  if (reactor_task_ == 0) return 0;
  return reactor_task_->get_reactor();
}

ACE_INLINE ACE_SOCK_Dgram&
RtpsUdpDataLink::unicast_socket()
{
  return unicast_socket_;
}

ACE_INLINE ACE_SOCK_Dgram_Mcast&
RtpsUdpDataLink::multicast_socket()
{
  return multicast_socket_;
}

} // namespace DCPS
} // namespace OpenDDS
