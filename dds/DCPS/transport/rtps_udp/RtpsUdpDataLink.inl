/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

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

ACE_INLINE bool
RtpsUdpDataLink::reactor_is_shut_down()
{
  if (reactor_task_ == 0) return true;
  return reactor_task_->is_shut_down();
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

ACE_INLINE void
RtpsUdpDataLink::release_remote_i(const RepoId& remote_id)
{
  locators_.erase(remote_id);
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
