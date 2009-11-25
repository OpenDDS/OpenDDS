/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DCPS_MULTICASTCONFIGURATION_H
#define DCPS_MULTICASTCONFIGURATION_H

#include "Multicast_Export.h"

#include "ace/INET_Addr.h"
#include "ace/Time_Value.h"

#include "dds/DCPS/transport/framework/TransportConfiguration.h"

namespace OpenDDS {
namespace DCPS {

// This constant forces the default group address selection to
// resolve port number 49152; this is the minimal port defined
// in the dynamic/private range [IANA 2009-11-16].
const TransportIdType DEFAULT_MULTICAST_ID(0xFFFFFF08);

class OpenDDS_Multicast_Export MulticastConfiguration
  : public TransportConfiguration {
public:
  MulticastConfiguration();

  virtual int load(const TransportIdType& id,
                   ACE_Configuration_Heap& config);

  /// the default group address selection is ipv4; this
  /// option controls whether ipv6 is to be used instead.
  /// the default value is false.
  bool default_to_ipv6_;
  
  /// The offset used to determine default port numbers;
  /// this value will be added to the transportId. The
  /// default value is: 49400 [IANA 2009-11-16].
  u_short port_offset_;

  /// The multicast group address from which to send and/or
  /// receive data. The default group addresses are:
  ///   224.0.0.128:<port> [IANA 2009-11-17], and
  ///    [FF01::80]:<port> [IANA 2009-08-28]
  ACE_INET_Addr group_address_;

  /// Enable/disable reliable communication. This option
  /// will eventually be deprecated once the ETF is able
  /// to properly segregate reliable/best-effort samples
  /// on a per-DataLink basis. The default value is true.
  bool reliable_;

  /// The maximum number of milliseconds to wait between
  /// SYN attempts during association (reliable only).
  /// The default value is 500.
  ACE_Time_Value syn_interval_;

  /// The maximum number of milliseconds to wait for a
  /// SYNACK response during association (reliable only).
  /// The default value is 30000 (30 seconds).
  ACE_Time_Value syn_timeout_;

  /// The maximum number of milliseconds to wait between
  /// NAK requests (reliable only). The default value is
  /// 2000 (2 seconds).
  ACE_Time_Value nak_interval_;

  /// The maximum number of milliseconds to wait before
  /// giving up on a NAK response (reliable only). The
  /// default value is 30000 (30 seconds).
  ACE_Time_Value nak_timeout_;

  /// The number of DCPS datagrams to retain in order to
  /// service incoming NAK requests (reliable only). The
  /// default value is 32; this yields a minimum of 32
  /// samples and a maximum 2048K of sample data.
  size_t nak_repair_size_;

private:
  void default_group_address(ACE_INET_Addr& group_address,
                             const TransportIdType& id);
};

} // namespace DCPS
} // namespace OpenDDS

#endif  /* DCPS_MULTICASTCONFIGURATION_H */
