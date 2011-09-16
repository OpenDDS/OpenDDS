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
  this->config_ = config;
  this->reactor_task_ = TransportReactorTask_rch(reactor_task, false);
}

ACE_INLINE void
RtpsUdpDataLink::send_strategy(RtpsUdpSendStrategy* send_strategy)
{
  this->send_strategy_ = send_strategy;
}

ACE_INLINE void
RtpsUdpDataLink::receive_strategy(RtpsUdpReceiveStrategy* recv_strategy)
{
  this->recv_strategy_ = recv_strategy;
}

ACE_INLINE bool
RtpsUdpDataLink::active() const
{
  return this->active_;
}

ACE_INLINE RtpsUdpInst*
RtpsUdpDataLink::config()
{
  return this->config_;
}

ACE_INLINE TransportReactorTask*
RtpsUdpDataLink::reactor_task()
{
  return TransportReactorTask_rch(this->reactor_task_)._retn();
}

ACE_INLINE ACE_Reactor*
RtpsUdpDataLink::get_reactor()
{
  if (this->reactor_task_ == 0) return 0;
  return this->reactor_task_->get_reactor();
}

ACE_INLINE ACE_SOCK_Dgram&
RtpsUdpDataLink::socket()
{
  return this->socket_;
}

} // namespace DCPS
} // namespace OpenDDS
