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
UdpDataLink::configure(UdpInst* config,
                       TransportReactorTask* reactor_task)
{
  this->config_ = config;
  this->reactor_task_ = reactor_task;
}

ACE_INLINE bool
UdpDataLink::active() const
{
  return this->active_;
}

ACE_INLINE UdpInst*
UdpDataLink::config()
{
  return this->config_;
}

ACE_INLINE TransportReactorTask*
UdpDataLink::reactor_task()
{
  return this->reactor_task_;
}

ACE_INLINE ACE_Reactor*
UdpDataLink::get_reactor()
{
  if (this->reactor_task_ == 0) return 0;
  return this->reactor_task_->get_reactor();
}

ACE_INLINE ACE_INET_Addr&
UdpDataLink::remote_address()
{
  return this->remote_address_;
}

ACE_INLINE ACE_SOCK_Dgram&
UdpDataLink::socket()
{
  return this->socket_;
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
