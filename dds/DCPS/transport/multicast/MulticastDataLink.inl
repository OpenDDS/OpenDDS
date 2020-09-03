/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

ACE_INLINE MulticastTransport&
MulticastDataLink::transport()
{
  return static_cast<MulticastTransport&>(DataLink::impl());
}

ACE_INLINE MulticastPeer
MulticastDataLink::local_peer() const
{
  return this->local_peer_;
}

ACE_INLINE MulticastSendStrategy*
MulticastDataLink::send_strategy()
{
  return this->send_strategy_.in();
}

ACE_INLINE MulticastReceiveStrategy*
MulticastDataLink::receive_strategy()
{
  return this->recv_strategy_.in();
}

ACE_INLINE SingleSendBuffer*
MulticastDataLink::send_buffer()
{
  return this->send_buffer_.get();
}

ACE_INLINE MulticastInst&
MulticastDataLink::config()
{
  return transport().config();
}

ACE_INLINE ReactorTask_rch
MulticastDataLink::reactor_task()
{
  return this->reactor_task_;
}

ACE_INLINE ACE_Reactor*
MulticastDataLink::get_reactor()
{
  if (this->reactor_task_ == 0) return 0;
  return this->reactor_task_->get_reactor();
}

ACE_INLINE ACE_Proactor*
MulticastDataLink::get_proactor()
{
  if (this->reactor_task_ == 0) return 0;
  return this->reactor_task_->get_proactor();
}

ACE_INLINE ACE_SOCK_Dgram_Mcast&
MulticastDataLink::socket()
{
  return this->socket_;
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
