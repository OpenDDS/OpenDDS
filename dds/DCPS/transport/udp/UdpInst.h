/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DCPS_UDPINST_H
#define DCPS_UDPINST_H

#include "Udp_Export.h"
#include "UdpTransport.h"

#include "ace/INET_Addr.h"

#include "dds/DCPS/transport/framework/TransportInst.h"

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Udp_Export UdpInst : public TransportInst {
public:
  /// The address from which to send/receive data.
  /// The default value is: none.
  ACE_INET_Addr local_address_;

  virtual int load(ACE_Configuration_Heap& cf,
                   ACE_Configuration_Section_Key& sect);

  /// Diagnostic aid.
  virtual void dump(std::ostream& os);

private:
  friend class UdpType;
  explicit UdpInst(const std::string& name);

  UdpTransport* new_impl(const TransportInst_rch& inst);
};

} // namespace DCPS
} // namespace OpenDDS

#endif  /* DCPS_UDPINST_H */
