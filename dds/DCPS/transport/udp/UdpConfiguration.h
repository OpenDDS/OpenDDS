/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DCPS_UDPCONFIGURATION_H
#define DCPS_UDPCONFIGURATION_H

#include "Udp_Export.h"

#include "ace/INET_Addr.h"

#include "dds/DCPS/transport/framework/TransportConfiguration.h"

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Udp_Export UdpConfiguration
  : public TransportConfiguration {
public:
  /// The address from which to send/receive data.
  /// The default value is: none.
  ACE_INET_Addr local_address_;

  UdpConfiguration();

  virtual int load(const TransportIdType& id,
                   ACE_Configuration_Heap& config);

  /// Diagnostic aid.
  virtual void dump();
};

} // namespace DCPS
} // namespace OpenDDS

#endif  /* DCPS_UDPCONFIGURATION_H */
