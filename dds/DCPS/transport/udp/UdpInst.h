/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TRANSPORT_UDP_UDPINST_H
#define OPENDDS_DCPS_TRANSPORT_UDP_UDPINST_H

#include "Udp_Export.h"
#include "UdpTransport.h"

#include <dds/DCPS/LogAddr.h>
#include "dds/DCPS/transport/framework/TransportInst.h"
#include "dds/DCPS/SafetyProfileStreams.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Udp_Export UdpInst : public TransportInst {
public:
  ConfigValue<UdpInst, ACE_INT32> send_buffer_size_;
  void send_buffer_size(ACE_INT32 sbs);
  ACE_INT32 send_buffer_size() const;

  ConfigValue<UdpInst, ACE_INT32> rcv_buffer_size_;
  void rcv_buffer_size(ACE_INT32 rbs);
  ACE_INT32 rcv_buffer_size() const;

  /// Diagnostic aid.
  virtual OPENDDS_STRING dump_to_str(DDS::DomainId_t) const;

  bool is_reliable() const { return false; }

  void local_address(const String& la);
  void local_address(const ACE_INET_Addr& addr)
  {
    local_address(LogAddr(addr).str());
  }
  void local_address(u_short port_number, const char* host_name)
  {
    local_address(String(host_name) + ":" + to_dds_string(port_number));
  }
  void local_address_set_port(u_short port_number) {
    String addr = local_address();
    set_port_in_addr_string(addr, port_number);
    local_address(addr);
  }
  String local_address() const;

  ACE_INET_Addr send_receive_address() const;

  bool set_locator_address(const ACE_INET_Addr& address);

  std::string get_locator_address() const
  {
    return locator_address_;
  }

  virtual size_t populate_locator(OpenDDS::DCPS::TransportLocator& trans_info,
                                  ConnectionInfoFlags flags,
                                  DDS::DomainId_t domain) const;

private:
  friend class UdpType;
  friend class UdpDataLink;
  template <typename T, typename U>
  friend RcHandle<T> OpenDDS::DCPS::make_rch(U const&);
  explicit UdpInst(const std::string& name);

  TransportImpl_rch new_impl(DDS::DomainId_t domain);

  std::string locator_address_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif  /* DCPS_UDPINST_H */
