/*
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
#include "dds/DCPS/SafetyProfileStreams.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Udp_Export UdpInst : public TransportInst {
public:
  ACE_INT32 send_buffer_size_;
  ACE_INT32 rcv_buffer_size_;

  virtual int load(ACE_Configuration_Heap& cf,
                   ACE_Configuration_Section_Key& sect);

  /// Diagnostic aid.
  virtual OPENDDS_STRING dump_to_str();

  bool is_reliable() const { return false; }

  virtual size_t populate_locator(OpenDDS::DCPS::TransportLocator& trans_info) const;

  OPENDDS_STRING local_address_string() const { return local_address_config_str_; }
  ACE_INET_Addr local_address() const { return local_address_; }
  void local_address(const char* str)
  {
    local_address_config_str_ = str;
    local_address_.set(str);
  }
  void local_address(u_short port_number, const char* host_name)
  {
    local_address_config_str_ = host_name;
    local_address_config_str_ += ":" + to_dds_string(port_number);
    local_address_.set(port_number, host_name);
  }
  void local_address_set_port(u_short port_number) {
    local_address_.set_port_number(port_number);
    size_t pos = local_address_config_str_.find_last_of(":");
    OPENDDS_STRING host_name = local_address_config_str_.substr(0, pos);
    local_address_config_str_ = host_name + ":" + to_dds_string(port_number);
  }

private:
  friend class UdpType;
  friend class UdpDataLink;
  explicit UdpInst(const std::string& name);

  UdpTransport* new_impl(const TransportInst_rch& inst);

  /// The address from which to send/receive data.
  /// The default value is: none.
  ACE_INET_Addr local_address_;
  OPENDDS_STRING local_address_config_str_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif  /* DCPS_UDPINST_H */
