/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "Tcp_pch.h"
#include "TcpInst.h"

#include "dds/DCPS/transport/framework/NetworkAddress.h"

#include "ace/Configuration.h"

#include <iostream>
#include <sstream>

#if !defined (__ACE_INLINE__)
#include "TcpInst.inl"
#endif /* __ACE_INLINE__ */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

OpenDDS::DCPS::TcpInst::~TcpInst()
{
  DBG_ENTRY_LVL("TcpInst","~TcpInst",6);
}

OpenDDS::DCPS::TransportImpl_rch
OpenDDS::DCPS::TcpInst::new_impl()
{
  return make_rch<TcpTransport>(ref(*this));
}

int
OpenDDS::DCPS::TcpInst::load(ACE_Configuration_Heap& cf,
                             ACE_Configuration_Section_Key& trans_sect)
{
  TransportInst::load(cf, trans_sect);

  std::string local_address;
  GET_CONFIG_STRING_VALUE(cf, trans_sect, ACE_TEXT("local_address"), local_address);

  if (local_address != "") {
    this->local_address(local_address.c_str());
  }

  GET_CONFIG_STRING_VALUE(cf, trans_sect, ACE_TEXT("pub_address"), pub_address_str_);

  GET_CONFIG_VALUE(cf, trans_sect, ACE_TEXT("enable_nagle_algorithm"),
                   this->enable_nagle_algorithm_, bool)

  GET_CONFIG_VALUE(cf, trans_sect, ACE_TEXT("conn_retry_initial_delay"),
                   this->conn_retry_initial_delay_, int)

  GET_CONFIG_DOUBLE_VALUE(cf, trans_sect, ACE_TEXT("conn_retry_backoff_multiplier"),
                   this->conn_retry_backoff_multiplier_)

  GET_CONFIG_VALUE(cf, trans_sect, ACE_TEXT("conn_retry_attempts"),
                   this->conn_retry_attempts_, int)

  GET_CONFIG_VALUE(cf, trans_sect, ACE_TEXT("passive_reconnect_duration"),
                   this->passive_reconnect_duration_, int)

  GET_CONFIG_VALUE(cf, trans_sect, ACE_TEXT("max_output_pause_period"),
                   this->max_output_pause_period_, int)

  GET_CONFIG_VALUE(cf, trans_sect, ACE_TEXT("active_conn_timeout_period"),
                   this->active_conn_timeout_period_, int)

  return 0;
}

OPENDDS_STRING
OpenDDS::DCPS::TcpInst::dump_to_str() const
{
  std::ostringstream os;
  os << TransportInst::dump_to_str() << std::endl;

  os << formatNameForDump("local_address")                 << this->local_address_string() << std::endl;
  os << formatNameForDump("pub_address")                   << this->pub_address_str_ << std::endl;
  os << formatNameForDump("enable_nagle_algorithm")        << (this->enable_nagle_algorithm_ ? "true" : "false") << std::endl;
  os << formatNameForDump("conn_retry_initial_delay")      << this->conn_retry_initial_delay_ << std::endl;
  os << formatNameForDump("conn_retry_backoff_multiplier") << this->conn_retry_backoff_multiplier_ << std::endl;
  os << formatNameForDump("conn_retry_attempts")           << this->conn_retry_attempts_ << std::endl;
  os << formatNameForDump("passive_reconnect_duration")    << this->passive_reconnect_duration_ << std::endl;
  os << formatNameForDump("max_output_pause_period")       << this->max_output_pause_period_ << std::endl;
  os << formatNameForDump("active_conn_timeout_period")    << this->active_conn_timeout_period_ << std::endl;
  return OPENDDS_STRING(os.str());
}

size_t
OpenDDS::DCPS::TcpInst::populate_locator(OpenDDS::DCPS::TransportLocator& local_info, ConnectionInfoFlags) const
{
  if (this->local_address() != ACE_INET_Addr() || !pub_address_str_.empty()) {
    // Get the public address string from the inst (usually the local address)
    NetworkAddress network_order_address(this->get_public_address());

    ACE_OutputCDR cdr;
    cdr << network_order_address;
    const CORBA::ULong len = static_cast<CORBA::ULong>(cdr.total_length());
    char* buffer = const_cast<char*>(cdr.buffer()); // safe

    local_info.transport_type = "tcp";
    local_info.data = TransportBLOB(len, len,
                                    reinterpret_cast<CORBA::Octet*>(buffer));
    return 1;
  } else {
    return 0;
  }
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
