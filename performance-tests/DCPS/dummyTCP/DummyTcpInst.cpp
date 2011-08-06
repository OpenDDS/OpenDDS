// -*- C++ -*-
//
// $Id$
#include "DummyTcp_pch.h"
#include "DummyTcpInst.h"
#include "DummyTcpTransport.h"

#include "ace/Configuration.h"

#include <iostream>

#if !defined (__ACE_INLINE__)
#include "DummyTcpInst.inl"
#endif /* __ACE_INLINE__ */


OpenDDS::DCPS::DummyTcpInst::~DummyTcpInst()
{
  DBG_ENTRY_LVL("DummyTcpInst","~DummyTcpInst",5);
}

int
OpenDDS::DCPS::DummyTcpInst::load(ACE_Configuration_Heap&,
                                  ACE_Configuration_Section_Key&)
{
  return 0;
}


OpenDDS::DCPS::TransportImpl*
OpenDDS::DCPS::DummyTcpInst::new_impl(const TransportInst_rch& inst)
{
  return new DummyTcpTransport(inst);
}


void
OpenDDS::DCPS::DummyTcpInst::dump(std::ostream& os)
{
  TransportInst::dump(os);

  os << formatNameForDump("local_address")                 << this->local_address_str_ << std::endl;
  os << formatNameForDump("enable_nagle_algorithm")        << (this->enable_nagle_algorithm_ ? "true" : "false") << std::endl;
  os << formatNameForDump("conn_retry_initial_delay")      << this->conn_retry_initial_delay_ << std::endl;
  os << formatNameForDump("conn_retry_backoff_multiplier") << this->conn_retry_backoff_multiplier_ << std::endl;
  os << formatNameForDump("conn_retry_attempts")           << this->conn_retry_attempts_ << std::endl;
  os << formatNameForDump("passive_reconnect_duration")    << this->passive_reconnect_duration_ << std::endl;
  os << formatNameForDump("passive_connect_duration")      << this->passive_connect_duration_ << std::endl;
  os << formatNameForDump("max_output_pause_period")       << this->max_output_pause_period_ << std::endl;
}
