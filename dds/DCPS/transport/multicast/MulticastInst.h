/*
 * $Id$
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

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Multicast_Export MulticastInst
  : public TransportInst {
public:
  /// Enables IPv6 default group address selection.
  /// The default value is: false.
  bool default_to_ipv6_;

  /// The offset used to determine default port numbers; this value
  /// will be added to the transport ID for the actual port number.
  /// The default value is: 49400 [IANA 2009-11-16].
  u_short port_offset_;

  /// The multicast group to join to send/receive data.
  /// The default value is:
  ///   224.0.0.128:<port> [IANA 2009-11-17], or
  ///    [FF01::80]:<port> [IANA 2009-08-28]
  ACE_INET_Addr group_address_;

  /// Enables reliable communication. This option will eventually
  /// be deprecated.
  /// The default value is: true.
  bool reliable_;

  /// The exponential base used during handshake retries; smaller
  /// values yield shorter delays between attempts (reliable only).
  /// The default value is: 2.0.
  double syn_backoff_;

  /// The minimum number of milliseconds to wait between handshake
  /// attempts during association (reliable only).
  /// The default value is: 250.
  ACE_Time_Value syn_interval_;

  /// The maximum number of milliseconds to wait before giving up
  /// on a handshake response during association (reliable only).
  /// The default value is: 30000 (30 seconds).
  ACE_Time_Value syn_timeout_;

  /// The number of datagrams to retain in order to service repair
  /// requests (reliable only).
  /// The default value is: 32.
  size_t nak_depth_;

  /// The minimum number of milliseconds to wait between repair
  /// requests (reliable only).
  /// The default value is: 500.
  ACE_Time_Value nak_interval_;

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
  ACE_Time_Value nak_timeout_;

  /// time-to-live.
  /// The default value is: 1 (in same subnet)
  char ttl_;

  /// The size of the socket receive buffer.
  /// The default value is: ACE_DEFAULT_MAX_SOCKET_BUFSIZ if it's defined,
  /// otherwise, 0.
  /// If the value is 0, the system default value is used.
  size_t rcv_buffer_size_;

  /// TODO: Remove
  virtual int load(const TransportIdType& id,
                   ACE_Configuration_Heap& cf);
  virtual int load(ACE_Configuration_Heap& cf,
                   ACE_Configuration_Section_Key& sect);

  /// Diagnostic aid.
  virtual void dump(std::ostream& os);

private:
  friend class MulticastType;
  explicit MulticastInst(const std::string& name);

  void default_group_address(ACE_INET_Addr& group_address,
                             const TransportIdType& id);

  MulticastTransport* new_impl(const TransportInst_rch& inst);
};

} // namespace DCPS
} // namespace OpenDDS

#endif  /* DCPS_MULTICASTINST_H */
