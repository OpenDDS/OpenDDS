/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TRANSPORT_RTPS_UDP_RTPSUDPINST_H
#define OPENDDS_DCPS_TRANSPORT_RTPS_UDP_RTPSUDPINST_H

#include "Rtps_Udp_Export.h"

#include "dds/DCPS/transport/framework/TransportInst.h"
#include "dds/DCPS/SafetyProfileStreams.h"
#include "dds/DCPS/NetworkAddress.h"

#include "dds/DCPS/RTPS/ICE/Ice.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace RTPS {
  class Sedp;
}

namespace DCPS {
class RtpsUdpTransport;
class TransportReceiveListener;
typedef RcHandle<TransportReceiveListener> TransportReceiveListener_rch;

class OpenDDS_Rtps_Udp_Export RtpsUdpInst : public TransportInst {
public:
  ACE_INT32 send_buffer_size_;
  ACE_INT32 rcv_buffer_size_;

  bool use_multicast_;
  unsigned char ttl_;
  OPENDDS_STRING multicast_interface_;

  size_t anticipated_fragments_;
  size_t max_message_size_;
  size_t nak_depth_;
  TimeDuration nak_response_delay_;
  TimeDuration heartbeat_period_;
  TimeDuration receive_address_duration_;
  bool responsive_mode_;
  TimeDuration send_delay_;

  virtual int load(ACE_Configuration_Heap& cf,
                   ACE_Configuration_Section_Key& sect);

  /// Diagnostic aid.
  virtual OPENDDS_STRING dump_to_str() const;

  bool is_reliable() const { return true; }
  bool requires_cdr_encapsulation() const { return true; }

  virtual size_t populate_locator(OpenDDS::DCPS::TransportLocator& trans_info, ConnectionInfoFlags flags) const;
  const TransportBLOB* get_blob(const OpenDDS::DCPS::TransportLocatorSeq& trans_info) const;

  NetworkAddress multicast_group_address() const { return multicast_group_address_; }
  void multicast_group_address(const NetworkAddress& addr)
  {
    if (addr.get_type() == AF_INET) {
      multicast_group_address_ = addr;
    } else {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: RtpsUdpInst::multicast_group_address set failed because address family is not AF_INET\n")));
    }
  }

  NetworkAddress local_address() const { return local_address_; }
  void local_address(const NetworkAddress& addr)
  {
    if (addr.get_type() == AF_INET) {
      local_address_ = addr;
    } else {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: RtpsUdpInst::local_address set failed because address family is not AF_INET\n")));
    }
  }

  NetworkAddress advertised_address() const { return advertised_address_; }
  void advertised_address(const NetworkAddress& addr)
  {
    if (addr.get_type() == AF_INET) {
      advertised_address_ = addr;
    } else {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: RtpsUdpInst::advertised_address set failed because address family is not AF_INET\n")));
    }
  }

#ifdef ACE_HAS_IPV6
  NetworkAddress ipv6_multicast_group_address() const { return ipv6_multicast_group_address_; }
  void ipv6_multicast_group_address(const NetworkAddress& addr)
  {
    if (addr.get_type() == AF_INET6) {
      ipv6_multicast_group_address_ = addr;
    } else {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: RtpsUdpInst::ipv6_multicast_group_address set failed because address family is not AF_INET6\n")));
    }
  }

  NetworkAddress ipv6_local_address() const { return ipv6_local_address_; }
  void ipv6_local_address(const NetworkAddress& addr)
  {
    if (addr.get_type() == AF_INET6) {
      ipv6_local_address_ = addr;
    } else {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: RtpsUdpInst::ipv6_local_address set failed because address family is not AF_INET6\n")));
    }
  }

  NetworkAddress ipv6_advertised_address() const { return ipv6_advertised_address_; }
  void ipv6_advertised_address(const NetworkAddress& addr)
  {
    if (addr.get_type() == AF_INET6) {
      ipv6_advertised_address_ = addr;
    } else {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: RtpsUdpInst::ipv6_advertised_address set failed because address family is not AF_INET6\n")));
    }
  }
#endif

  /// Relay address and stun server address may change, these use a mutex
  ///{
  void rtps_relay_only(bool flag);
  bool rtps_relay_only() const;
  void use_rtps_relay(bool flag);
  bool use_rtps_relay() const;
  void rtps_relay_address(const NetworkAddress& address);
  NetworkAddress rtps_relay_address() const;
  void use_ice(bool flag);
  bool use_ice() const;
  void stun_server_address(const NetworkAddress& address);
  NetworkAddress stun_server_address() const;
  ///}

  void update_locators(const RepoId& remote_id,
                       const TransportLocatorSeq& locators);

  void get_last_recv_locator(const RepoId& /*remote_id*/,
                             TransportLocator& /*locators*/);

  void rtps_relay_address_change();
  void append_transport_statistics(TransportStatisticsSequence& seq);

private:
  friend class RtpsUdpType;
  template <typename T, typename U>
  friend RcHandle<T> OpenDDS::DCPS::make_rch(U const&);
  explicit RtpsUdpInst(const OPENDDS_STRING& name);

  TransportImpl_rch new_impl();

  friend class RTPS::Sedp;
  friend class RtpsUdpTransport;
  TransportReceiveListener_rch opendds_discovery_default_listener_;
  RepoId opendds_discovery_guid_;

  NetworkAddress multicast_group_address_;
  NetworkAddress local_address_;
  NetworkAddress advertised_address_;
#ifdef ACE_HAS_IPV6
  NetworkAddress ipv6_multicast_group_address_;
  NetworkAddress ipv6_local_address_;
  NetworkAddress ipv6_advertised_address_;
#endif

  mutable ACE_SYNCH_MUTEX config_lock_;
  bool rtps_relay_only_;
  bool use_rtps_relay_;
  NetworkAddress rtps_relay_address_;
  bool use_ice_;
  NetworkAddress stun_server_address_;
};

inline void RtpsUdpInst::rtps_relay_only(bool flag)
{
  ACE_GUARD(ACE_Thread_Mutex, g, config_lock_);
  rtps_relay_only_ = flag;
  if (DCPS::DCPS_debug_level > 3) {
    ACE_DEBUG((LM_INFO, "(%P|%t) RtpsUdpInst::rtps_relay_only is now %d\n", rtps_relay_only_));
  }
}

inline bool RtpsUdpInst::rtps_relay_only() const
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, config_lock_, false);
  return rtps_relay_only_;
}

inline void RtpsUdpInst::use_rtps_relay(bool flag)
{
  ACE_GUARD(ACE_Thread_Mutex, g, config_lock_);
  use_rtps_relay_ = flag;
  if (DCPS::DCPS_debug_level > 3) {
    ACE_DEBUG((LM_INFO, "(%P|%t) RtpsUdpInst::use_rtps_relay is now %d\n", use_rtps_relay_));
  }
}

inline bool RtpsUdpInst::use_rtps_relay() const
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, config_lock_, false);
  return use_rtps_relay_;
}

inline void RtpsUdpInst::rtps_relay_address(const NetworkAddress& address)
{
  ACE_GUARD(ACE_Thread_Mutex, g, config_lock_);
  rtps_relay_address_ = address;
}

inline NetworkAddress RtpsUdpInst::rtps_relay_address() const
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, config_lock_, NetworkAddress());
  return rtps_relay_address_;
}

inline void RtpsUdpInst::use_ice(bool flag)
{
  ACE_GUARD(ACE_Thread_Mutex, g, config_lock_);
  use_ice_ = flag;
  if (DCPS::DCPS_debug_level > 3) {
    ACE_DEBUG((LM_INFO, "(%P|%t) RtpsUdpInst::use_ice is now %d\n", use_ice_));
  }
}

inline bool RtpsUdpInst::use_ice() const
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, config_lock_, false);
  return use_ice_;
}

inline void RtpsUdpInst::stun_server_address(const NetworkAddress& address)
{
  ACE_GUARD(ACE_Thread_Mutex, g, config_lock_);
  stun_server_address_ = address;
}

inline NetworkAddress RtpsUdpInst::stun_server_address() const
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, config_lock_, NetworkAddress());
  return stun_server_address_;
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif  /* DCPS_RTPSUDPINST_H */
