/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "Tcp_pch.h"
#include "TcpInst.h"
#include <iostream>

#if !defined (__ACE_INLINE__)
#include "TcpInst.inl"
#endif /* __ACE_INLINE__ */

OpenDDS::DCPS::TcpInst::~TcpInst()
{
  DBG_ENTRY_LVL("TcpInst","~TcpInst",6);
}

int
OpenDDS::DCPS::TcpInst::load(const TransportIdType& id,
                                            ACE_Configuration_Heap& cf)
{
  // The default transport can not be configured by user.
  if (id == DEFAULT_TCP_ID) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) You can not configure the default Tcp transport(id=%u) !!!\n"),
               id));
    return -1;
  }

  TransportInst::load(id, cf);

  ACE_TString sect_name = id_to_section_name(id);
  const ACE_Configuration_Section_Key &root = cf.root_section();
  ACE_Configuration_Section_Key trans_sect;

  if (cf.open_section(root, sect_name.c_str(), 0, trans_sect) != 0)
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("Failed to open section \"%s\" \n"),
                      sect_name.c_str()), -1);

  ACE_TString local_address;
  GET_CONFIG_STRING_VALUE(cf, trans_sect, ACE_TEXT("local_address"), local_address);

  if (local_address != ACE_TEXT("")) {
    this->local_address_str_ = local_address;
    this->local_address_.set(local_address_str_.c_str());
  }

  GET_CONFIG_VALUE(cf, trans_sect, ACE_TEXT("enable_nagle_algorithm"),
                   this->enable_nagle_algorithm_, bool)

  GET_CONFIG_VALUE(cf, trans_sect, ACE_TEXT("conn_retry_initial_delay"),
                   this->conn_retry_initial_delay_, int)

  GET_CONFIG_VALUE(cf, trans_sect, ACE_TEXT("conn_retry_backoff_multiplier"),
                   this->conn_retry_backoff_multiplier_, double)

  GET_CONFIG_VALUE(cf, trans_sect, ACE_TEXT("conn_retry_attempts"),
                   this->conn_retry_attempts_, int)

  GET_CONFIG_VALUE(cf, trans_sect, ACE_TEXT("passive_reconnect_duration"),
                   this->passive_reconnect_duration_, int)

  GET_CONFIG_VALUE(cf, trans_sect, ACE_TEXT("passive_connect_duration"),
                   this->passive_connect_duration_, unsigned int)

  GET_CONFIG_VALUE(cf, trans_sect, ACE_TEXT("max_output_pause_period"),
                   this->max_output_pause_period_, int)

  return 0;
}

void
OpenDDS::DCPS::TcpInst::dump(std::ostream& os)
{
  TransportInst::dump(os);

  os << formatNameForDump(ACE_TEXT("local_address"))                 << this->local_address_str_ << std::endl;
  os << formatNameForDump(ACE_TEXT("enable_nagle_algorithm"))        << (this->enable_nagle_algorithm_ ? "true" : "false") << std::endl;
  os << formatNameForDump(ACE_TEXT("conn_retry_initial_delay"))      << this->conn_retry_initial_delay_ << std::endl;
  os << formatNameForDump(ACE_TEXT("conn_retry_backoff_multiplier")) << this->conn_retry_backoff_multiplier_ << std::endl;
  os << formatNameForDump(ACE_TEXT("conn_retry_attempts"))           << this->conn_retry_attempts_ << std::endl;
  os << formatNameForDump(ACE_TEXT("passive_reconnect_duration"))    << this->passive_reconnect_duration_ << std::endl;
  os << formatNameForDump(ACE_TEXT("passive_connect_duration"))      << this->passive_connect_duration_ << std::endl;
  os << formatNameForDump(ACE_TEXT("max_output_pause_period"))       << this->max_output_pause_period_ << std::endl;
}
