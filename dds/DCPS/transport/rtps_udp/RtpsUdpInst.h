/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TRANSPORT_RTPS_UDP_RTPSUDPINST_H
#define OPENDDS_DCPS_TRANSPORT_RTPS_UDP_RTPSUDPINST_H

#include "Rtps_Udp_Export.h"
#include "RtpsUdpTransport_rch.h"

#include <dds/DCPS/NetworkAddress.h>
#include <dds/DCPS/SafetyProfileStreams.h>
#include <dds/DCPS/RTPS/ICE/Ice.h>
#include <dds/DCPS/transport/framework/TransportInst.h>

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

  static const suseconds_t DEFAULT_NAK_RESPONSE_DELAY_USEC = 200000; // default from RTPS
  static const time_t DEFAULT_HEARTBEAT_PERIOD_SEC = 1; // no default in RTPS spec

  ConfigValue<RtpsUdpInst, ACE_INT32> send_buffer_size_;
  void send_buffer_size(ACE_INT32 sbs);
  ACE_INT32 send_buffer_size() const;

  ConfigValue<RtpsUdpInst, ACE_INT32> rcv_buffer_size_;
  void rcv_buffer_size(ACE_INT32 rbs);
  ACE_INT32 rcv_buffer_size() const;

  ConfigValue<RtpsUdpInst, bool> use_multicast_;
  void use_multicast(bool um);
  bool use_multicast() const;

  ConfigValue<RtpsUdpInst, unsigned char> ttl_;
  void ttl(unsigned char t);
  unsigned char ttl() const;

  ConfigValueRef<RtpsUdpInst, String> multicast_interface_;
  void multicast_interface(const String& mi);
  String multicast_interface() const;

  ConfigValue<RtpsUdpInst, size_t> anticipated_fragments_;
  void anticipated_fragments(size_t af);
  size_t anticipated_fragments() const;

  ConfigValue<RtpsUdpInst, size_t> max_message_size_;
  void max_message_size(size_t mms);
  size_t max_message_size() const;

  ConfigValue<RtpsUdpInst, size_t> nak_depth_;
  void nak_depth(size_t mms);
  size_t nak_depth() const;

  ConfigValueRef<RtpsUdpInst, TimeDuration> nak_response_delay_;
  void nak_response_delay(const TimeDuration& nrd);
  TimeDuration nak_response_delay() const;

  ConfigValueRef<RtpsUdpInst, TimeDuration> heartbeat_period_;
  void heartbeat_period(const TimeDuration& nrd);
  TimeDuration heartbeat_period() const;

  ConfigValueRef<RtpsUdpInst, TimeDuration> receive_address_duration_;
  void receive_address_duration(const TimeDuration& rad);
  TimeDuration receive_address_duration() const;

  ConfigValue<RtpsUdpInst, bool> responsive_mode_;
  void responsive_mode(bool rm);
  bool responsive_mode() const;

  ConfigValueRef<RtpsUdpInst, TimeDuration> send_delay_;
  void send_delay(const TimeDuration& sd);
  TimeDuration send_delay() const;

  virtual int load(ACE_Configuration_Heap& cf,
                   ACE_Configuration_Section_Key& sect);

  /// Diagnostic aid.
  virtual OPENDDS_STRING dump_to_str() const;

  bool is_reliable() const { return true; }
  bool requires_cdr_encapsulation() const { return true; }

  virtual size_t populate_locator(OpenDDS::DCPS::TransportLocator& trans_info, ConnectionInfoFlags flags) const;
  const TransportBLOB* get_blob(const OpenDDS::DCPS::TransportLocatorSeq& trans_info) const;

  void multicast_group_address(const NetworkAddress& addr);
  NetworkAddress multicast_group_address() const;

  void local_address(const NetworkAddress& addr);
  NetworkAddress local_address() const;

  void advertised_address(const NetworkAddress& addr);
  NetworkAddress advertised_address() const;

#ifdef ACE_HAS_IPV6
  void ipv6_multicast_group_address(const NetworkAddress& addr);
  NetworkAddress ipv6_multicast_group_address() const;

  void ipv6_local_address(const NetworkAddress& addr);
  NetworkAddress ipv6_local_address() const;

  void ipv6_advertised_address(const NetworkAddress& addr);
  NetworkAddress ipv6_advertised_address() const;
#endif

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

  void update_locators(const GUID_t& remote_id,
                       const TransportLocatorSeq& locators);

  void get_last_recv_locator(const GUID_t& /*remote_id*/,
                             TransportLocator& /*locators*/);

  void append_transport_statistics(TransportStatisticsSequence& seq);

private:
  friend class RtpsUdpType;
  template <typename T, typename U0, typename U1>
  friend RcHandle<T> OpenDDS::DCPS::make_rch(U0 const&, U1 const&);
  explicit RtpsUdpInst(const OPENDDS_STRING& name,
                       bool is_template);

  TransportImpl_rch new_impl();

  friend class RTPS::Sedp;
  friend class RtpsUdpTransport;
  TransportReceiveListener_rch opendds_discovery_default_listener_;
  GUID_t opendds_discovery_guid_;
  NetworkAddress actual_local_address_;
#ifdef ACE_HAS_IPV6
  NetworkAddress ipv6_actual_local_address_;
#endif
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_TRANSPORT_RTPS_UDP_RTPSUDPINST_H */
