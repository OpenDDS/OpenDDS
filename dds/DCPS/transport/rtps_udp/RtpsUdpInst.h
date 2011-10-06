/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DCPS_RTPSUDPINST_H
#define DCPS_RTPSUDPINST_H

#include "Rtps_Udp_Export.h"
#include "RtpsUdpTransport.h"

#include "dds/DCPS/transport/framework/TransportInst.h"

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Rtps_Udp_Export RtpsUdpInst : public TransportInst {
public:

  ACE_INET_Addr local_address_;

  bool use_multicast_;
  ACE_INET_Addr multicast_group_address_;

  virtual int load(ACE_Configuration_Heap& cf,
                   ACE_Configuration_Section_Key& sect);

  /// Diagnostic aid.
  virtual void dump(std::ostream& os);

  bool is_reliable() const { return true; }
  bool requires_cdr() const { return true; }

private:
  friend class RtpsUdpType;
  explicit RtpsUdpInst(const std::string& name);

  RtpsUdpTransport* new_impl(const TransportInst_rch& inst);
};

} // namespace DCPS
} // namespace OpenDDS

#endif  /* DCPS_RTPSUDPINST_H */
