/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "ace/INET_Addr.h"

#include "dds/DCPS/transport/framework/TransportConfiguration.h"

#include "Multicast_Export.h"

#ifndef DCPS_MULTICASTCONFIGURATION_H
#define DCPS_MULTICASTCONFIGURATION_H

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Multicast_Export MulticastConfiguration
  : public TransportConfiguration {
public:
  MulticastConfiguration();

  virtual int load(const TransportIdType& id,
                   ACE_Configuration_Heap& config);

  /// The default group address selection is IPv4; this
  /// option controls whether IPv6 is to be used instead.
  /// The default is false.
  bool default_to_ipv6_;

  /// The multicast group address from which to send and/or
  /// receive data. The default group addresses are:
  ///   224.0.0.128:<transportId> (IPv4), and
  ///   [FF01::80]:<transportId> (IPv6)
  ACE_INET_Addr group_address_;

  /// The maximum number of milliseconds to wait while
  /// handshaking with remote peers. The default is 30000
  /// (30 seconds).
  long handshake_timeout_;

  /// The maximum number of milliseconds to wait before
  /// giving up on NAK responses (reliable only). The
  /// default is 30000 (30 seconds).
  long nak_timeout_;

  /// The number of DCPS datagrams to retain in order to
  /// service incoming NAK requests (reliable only).
  /// The default is 32; this yields a minimum of 32
  /// samples and a maximum 2048K of sample data.
  size_t nak_repair_depth_;

private:
  void default_group_address(ACE_INET_Addr& group_address,
                             const TransportIdType& id);
};

} // namespace DCPS
} // namespace OpenDDS

#endif  /* DCPS_MULTICASTCONFIGURATION_H */
