/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TRANSPORT_MULTICAST_MULTICASTINST_H
#define OPENDDS_DCPS_TRANSPORT_MULTICAST_MULTICASTINST_H

#include "Multicast_Export.h"
#include "MulticastTransport.h"

#include "ace/INET_Addr.h"
#include "ace/Time_Value.h"

#include "dds/DCPS/transport/framework/TransportInst.h"

#include <string>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Multicast_Export MulticastInst
  : public TransportInst {
public:

  static const size_t DEFAULT_NAK_DEPTH = 32u;
  static const long DEFAULT_NAK_INTERVAL = 500;
  static const long DEFAULT_NAK_DELAY_INTERVALS = 4;
  static const long DEFAULT_NAK_MAX = 3;
  static const long DEFAULT_NAK_TIMEOUT = 30000;

  /// Enables IPv6 default group address selection.
  /// The default value is: false.
  ConfigValue<MulticastInst, bool> default_to_ipv6_;
  void default_to_ipv6(bool flag);
  bool default_to_ipv6() const;

  /// The default port number (when group_address is not set)
  /// The default value is: 49152 [IANA 2009-11-16].
  ConfigValue<MulticastInst, u_short> port_offset_;
  void port_offset(u_short po);
  u_short port_offset() const;

  /// The multicast group to join to send/receive data.
  /// The default value is:
  ///   224.0.0.128:<port> [IANA 2009-11-17], or
  ///    [FF01::80]:<port> [IANA 2009-08-28]
  ConfigValueRef<MulticastInst, NetworkAddress> group_address_;
  void group_address(const NetworkAddress& na);
  NetworkAddress group_address() const;

  /// If non-empty, the address to pass to ACE which indicates the
  /// local network interface which should be used for joining the
  /// multicast group.
  ConfigValueRef<MulticastInst, String> local_address_;
  void local_address(const String& la);
  String local_address() const;

  /// Enables reliable communication. This option will eventually
  /// be deprecated.
  /// The default value is: true.
  ConfigValue<MulticastInst, bool> reliable_;
  void reliable(bool flag);
  bool reliable() const;

  /// The exponential base used during handshake retries; smaller
  /// values yield shorter delays between attempts.
  /// The default value is: 2.0.
  ConfigValue<MulticastInst, double> syn_backoff_;
  void syn_backoff(double sb);
  double syn_backoff() const;

  /// The minimum number of milliseconds to wait between handshake
  /// attempts during association.
  /// The default value is: 250.
  ConfigValueRef<MulticastInst, TimeDuration> syn_interval_;
  void syn_interval(const TimeDuration& si);
  TimeDuration syn_interval() const;

  /// The maximum number of milliseconds to wait before giving up
  /// on a handshake response during association.
  /// The default value is: 30000 (30 seconds).
  ConfigValueRef<MulticastInst, TimeDuration> syn_timeout_;
  void syn_timeout(const TimeDuration& st);
  TimeDuration syn_timeout() const;

  /// The number of datagrams to retain in order to service repair
  /// requests (reliable only).
  /// The default value is: 32.
  ConfigValue<MulticastInst, size_t> nak_depth_;
  void nak_depth(size_t nd);
  size_t nak_depth() const;

  /// The minimum number of milliseconds to wait between repair
  /// requests (reliable only).
  /// The default value is: 500.
  ConfigValueRef<MulticastInst, TimeDuration> nak_interval_;
  void nak_interval(const TimeDuration& ni);
  TimeDuration nak_interval() const;

  /// The number of interval's between nak's for a sample
  /// (after initial nak).
  /// The default value is: 4.
  ConfigValue<MulticastInst, size_t> nak_delay_intervals_;
  void nak_delay_intervals(size_t ndi);
  size_t nak_delay_intervals() const;

  /// The maximum number of a missing sample will be nak'ed.
  /// The default value is: 3.
  ConfigValue<MulticastInst, size_t> nak_max_;
  void nak_max(size_t nm);
  size_t nak_max() const;

  /// The maximum number of milliseconds to wait before giving up
  /// on a repair response (reliable only).
  /// The default value is: 30000 (30 seconds).
  ConfigValueRef<MulticastInst, TimeDuration> nak_timeout_;
  void nak_timeout(const TimeDuration& nt);
  TimeDuration nak_timeout() const;

  /// time-to-live.
  /// The default value is: 1 (in same subnet)
  ConfigValue<MulticastInst, unsigned char> ttl_;
  void ttl(unsigned char t);
  unsigned char ttl() const;

  /// The size of the socket receive buffer.
  /// The default value is: ACE_DEFAULT_MAX_SOCKET_BUFSIZ if it's defined,
  /// otherwise, 0.
  /// If the value is 0, the system default value is used.
  ConfigValue<MulticastInst, size_t> rcv_buffer_size_;
  void rcv_buffer_size(size_t rbs);
  size_t rcv_buffer_size() const;

  /// Sending using asynchronous I/O on Windows platforms that support it.
  /// The default value is: false.
  /// This parameter has no effect on non-Windows platforms and Windows platforms
  /// that don't support asynchronous I/O.
  ConfigValue<MulticastInst, bool> async_send_;
  void async_send(bool as);
  bool async_send() const;

  /// Diagnostic aid.
  virtual OPENDDS_STRING dump_to_str(DDS::DomainId_t) const;

  bool is_reliable() const { return reliable(); }

  virtual size_t populate_locator(OpenDDS::DCPS::TransportLocator& trans_info,
                                  ConnectionInfoFlags flags,
                                  DDS::DomainId_t domain) const;

private:
  friend class MulticastType;
  template <typename T, typename U>
  friend RcHandle<T> OpenDDS::DCPS::make_rch(U const&);
  explicit MulticastInst(const std::string& name);

  TransportImpl_rch new_impl(DDS::DomainId_t domain);
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif  /* DCPS_MULTICASTINST_H */
