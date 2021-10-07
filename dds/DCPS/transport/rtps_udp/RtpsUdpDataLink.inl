/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {



ACE_INLINE ACE_Reactor*
RtpsUdpDataLink::get_reactor()
{
  if (!reactor_task_) return 0;
  return reactor_task_->get_reactor();
}

ACE_INLINE bool
RtpsUdpDataLink::reactor_is_shut_down()
{
  if (!reactor_task_) return true;
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

#ifdef ACE_HAS_IPV6
ACE_INLINE ACE_SOCK_Dgram&
RtpsUdpDataLink::ipv6_unicast_socket()
{
  return ipv6_unicast_socket_;
}

ACE_INLINE ACE_SOCK_Dgram_Mcast&
RtpsUdpDataLink::ipv6_multicast_socket()
{
  return ipv6_multicast_socket_;
}
#endif

#if defined(OPENDDS_SECURITY)
ACE_INLINE DDS::Security::ParticipantCryptoHandle
RtpsUdpDataLink::local_crypto_handle() const
{
  ACE_Guard<ACE_Thread_Mutex> guard(security_mutex_);
  return local_crypto_handle_;
}

ACE_INLINE void
RtpsUdpDataLink::local_crypto_handle(DDS::Security::ParticipantCryptoHandle h)
{
  ACE_Guard<ACE_Thread_Mutex> guard(security_mutex_);
  local_crypto_handle_ = h;
}
#endif

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
