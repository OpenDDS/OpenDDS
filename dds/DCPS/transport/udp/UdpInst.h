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
  bool local_address_set_port(u_short port_number) {
    local_address_.set_port_number(port_number);
    ACE_TCHAR buf[1024];
    if (local_address_.addr_to_string(buf, 1024) == 0) {
      local_address_config_str_ = ACE_TEXT_ALWAYS_CHAR(buf);
      return true;
    }
    else {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) UdpInst::local_address_set_port():")
                           ACE_TEXT(" failure to convert local IP address to text representation\n")));
      return false;
    }
  }

private:
  friend class UdpType;
  friend class UdpDataLink;
  template <typename T, typename U>
  friend RcHandle<T> OpenDDS::DCPS::make_rch(U const&);
  explicit UdpInst(const std::string& name);

  TransportImpl_rch new_impl(const TransportInst_rch& inst);

  /// The address from which to send/receive data.
  /// The default value is: none.
  ACE_INET_Addr local_address_;
  OPENDDS_STRING local_address_config_str_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif  /* DCPS_UDPINST_H */
