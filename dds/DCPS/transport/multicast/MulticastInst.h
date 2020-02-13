/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DCPS_MULTICASTINST_H
#define DCPS_MULTICASTINST_H

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
  /// Enables IPv6 default group address selection.
  /// The default value is: false.
  bool default_to_ipv6_;

  /// The default port number (when group_address is not set)
  /// The default value is: 49152 [IANA 2009-11-16].
  u_short port_offset_;

  /// The multicast group to join to send/receive data.
  /// The default value is:
  ///   224.0.0.128:<port> [IANA 2009-11-17], or
  ///    [FF01::80]:<port> [IANA 2009-08-28]
  ACE_INET_Addr group_address_;

  /// If non-empty, the address to pass to ACE which indicates the
  /// local network interface which should be used for joining the
  /// multicast group.
  std::string local_address_;

  /// Enables reliable communication. This option will eventually
  /// be deprecated.
  /// The default value is: true.
  bool reliable_;

  /// The exponential base used during handshake retries; smaller
  /// values yield shorter delays between attempts.
  /// The default value is: 2.0.
  double syn_backoff_;

  /// The minimum number of milliseconds to wait between handshake
  /// attempts during association.
  /// The default value is: 250.
  TimeDuration syn_interval_;

  /// The maximum number of milliseconds to wait before giving up
  /// on a handshake response during association.
  /// The default value is: 30000 (30 seconds).
  TimeDuration syn_timeout_;

  /// The number of datagrams to retain in order to service repair
  /// requests (reliable only).
  /// The default value is: 32.
  size_t nak_depth_;

  /// The minimum number of milliseconds to wait between repair
  /// requests (reliable only).
  /// The default value is: 500.
  TimeDuration nak_interval_;

  /// The number of interval's between nak's for a sample
  /// (after initial nak).
  /// The default value is: 4.
  size_t nak_delay_intervals_;

  /// The maximum number of a missing sample will be nak'ed.
  /// The default value is: 3.
  size_t nak_max_;

  /// The maximum number of milliseconds to wait before giving up
  /// on a repair response (reliable only).
  /// The default value is: 30000 (30 seconds).
  TimeDuration nak_timeout_;

  /// time-to-live.
  /// The default value is: 1 (in same subnet)
  unsigned char ttl_;

  /// The size of the socket receive buffer.
  /// The default value is: ACE_DEFAULT_MAX_SOCKET_BUFSIZ if it's defined,
  /// otherwise, 0.
  /// If the value is 0, the system default value is used.
  size_t rcv_buffer_size_;

  /// Sending using asynchronous I/O on Windows platforms that support it.
  /// The default value is: false.
  /// This parameter has no effect on non-Windows platforms and Windows platforms
  /// that don't support asynchronous I/O.
  bool async_send_;

  virtual int load(ACE_Configuration_Heap& cf,
                   ACE_Configuration_Section_Key& sect);

  /// Diagnostic aid.
  virtual OPENDDS_STRING dump_to_str() const;

  bool is_reliable() const { return this->reliable_; }

  bool async_send() const { return this->async_send_; }

  virtual size_t populate_locator(OpenDDS::DCPS::TransportLocator& trans_info, ConnectionInfoFlags flags) const;

private:
  friend class MulticastType;
  template <typename T, typename U>
  friend RcHandle<T> OpenDDS::DCPS::make_rch(U const&);
  explicit MulticastInst(const std::string& name);

  void default_group_address(ACE_INET_Addr& group_address);

  TransportImpl_rch new_impl();
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif  /* DCPS_MULTICASTINST_H */
