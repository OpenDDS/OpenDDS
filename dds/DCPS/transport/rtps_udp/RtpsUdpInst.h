/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DCPS_RTPSUDPINST_H
#define DCPS_RTPSUDPINST_H

#include "Rtps_Udp_Export.h"

#include "dds/DCPS/transport/framework/TransportInst.h"
#include "dds/DCPS/SafetyProfileStreams.h"

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
  ACE_INET_Addr multicast_group_address_;
  OPENDDS_STRING multicast_group_address_str_;
  OPENDDS_STRING multicast_interface_;

  size_t nak_depth_;
  size_t max_bundle_size_;
  TimeDuration nak_response_delay_, heartbeat_period_,
    heartbeat_response_delay_, handshake_timeout_, durable_data_timeout_;

  virtual int load(ACE_Configuration_Heap& cf,
                   ACE_Configuration_Section_Key& sect);

  /// Diagnostic aid.
  virtual OPENDDS_STRING dump_to_str() const;

  bool is_reliable() const { return true; }
  bool requires_cdr() const { return true; }

  virtual size_t populate_locator(OpenDDS::DCPS::TransportLocator& trans_info, ConnectionInfoFlags flags) const;
  const TransportBLOB* get_blob(const OpenDDS::DCPS::TransportLocatorSeq& trans_info) const;

  OPENDDS_STRING local_address_string() const { return local_address_config_str_; }
  ACE_INET_Addr local_address() const { return local_address_; }
  void local_address(const char* str)
  {
    local_address_config_str_ = str;
    local_address_.set(str);
  }
  void local_address(u_short port_number, const char* host_name)
  {
    local_address_config_str_ = host_name;
    local_address_config_str_ += ":" + to_dds_string(port_number);
    local_address_.set(port_number, host_name);
  }
  void local_address_set_port(u_short port_number) {
    local_address_.set_port_number(port_number);
    set_port_in_addr_string(local_address_config_str_, port_number);
  }

  /// Relay address and stun server address may change, these use a mutex
  ///{
  void rtps_relay_address(const ACE_INET_Addr& address);
  ACE_INET_Addr rtps_relay_address() const;
  void stun_server_address(const ACE_INET_Addr& address);
  ACE_INET_Addr stun_server_address() const;
  ///}

  TimeDuration rtps_relay_beacon_period_;
  bool use_rtps_relay_;
  bool rtps_relay_only_;
  bool use_ice_;

  void update_locators(const RepoId& remote_id,
                       const TransportLocatorSeq& locators);

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

  ACE_INET_Addr local_address_;
  OPENDDS_STRING local_address_config_str_;
  ACE_INET_Addr rtps_relay_address_;
  mutable ACE_SYNCH_MUTEX rtps_relay_config_lock_;
  ACE_INET_Addr stun_server_address_;
  mutable ACE_SYNCH_MUTEX stun_server_config_lock_;

#ifdef OPENDDS_SECURITY
  ICE::AddressListType host_addresses() const;
#endif
};

inline void RtpsUdpInst::rtps_relay_address(const ACE_INET_Addr& address)
{
  ACE_GUARD(ACE_Thread_Mutex, g, rtps_relay_config_lock_);
  rtps_relay_address_ = address;
}

inline ACE_INET_Addr RtpsUdpInst::rtps_relay_address() const
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, rtps_relay_config_lock_, ACE_INET_Addr());
  return rtps_relay_address_;
}

inline void RtpsUdpInst::stun_server_address(const ACE_INET_Addr& address)
{
  ACE_GUARD(ACE_Thread_Mutex, g, stun_server_config_lock_);
  stun_server_address_ = address;
}

inline ACE_INET_Addr RtpsUdpInst::stun_server_address() const
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, stun_server_config_lock_, ACE_INET_Addr());
  return stun_server_address_;
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif  /* DCPS_RTPSUDPINST_H */
