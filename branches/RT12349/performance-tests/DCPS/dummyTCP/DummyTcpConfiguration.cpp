// -*- C++ -*-
//
// $Id$
#include "DummyTcp_pch.h"
#include "DummyTcpConfiguration.h"


#if !defined (__ACE_INLINE__)
#include "DummyTcpConfiguration.inl"
#endif /* __ACE_INLINE__ */


OpenDDS::DCPS::DummyTcpConfiguration::~DummyTcpConfiguration()
{
  DBG_ENTRY_LVL("DummyTcpConfiguration","~DummyTcpConfiguration",5);
}

int
OpenDDS::DCPS::DummyTcpConfiguration::load (const TransportIdType& id,
                                         ACE_Configuration_Heap& cf)
{
  // The default transport can not be configured by user.
  if (id == DEFAULT_DUMMY_TCP_ID)
    {
      ACE_ERROR ((LM_ERROR, "(%P|%t)You can not configure the default DummyTcp transport(id=%u) !!! \n",
        id));
      return -1;
    }

  TransportConfiguration::load (id, cf);

  char section [50];
  ACE_OS::sprintf (section, "%s%d", TRANSPORT_SECTION_NAME_PREFIX, id);
  const ACE_Configuration_Section_Key &root = cf.root_section ();
  ACE_Configuration_Section_Key trans_sect;
  if (cf.open_section (root, section, 0, trans_sect) != 0)
    ACE_ERROR_RETURN ((LM_ERROR,
                       ACE_TEXT ("Failed to open section \"%s\" \n"), section),
                       -1);

  ACE_CString local_address;
  GET_CONFIG_STRING_VALUE (cf, trans_sect, "local_address", local_address);
  if (local_address != "")
  {
    this->local_address_.set (local_address.c_str ());
    this->local_address_str_ = local_address.c_str();
  }

  GET_CONFIG_VALUE (cf, trans_sect, "enable_nagle_algorithm",
    this->enable_nagle_algorithm_, bool)

  GET_CONFIG_VALUE (cf, trans_sect, "conn_retry_initial_delay",
    this->conn_retry_initial_delay_, int)

  GET_CONFIG_DOUBLE_VALUE (cf, trans_sect, "conn_retry_backoff_multiplier",
    this->conn_retry_backoff_multiplier_)

  GET_CONFIG_VALUE (cf, trans_sect, "conn_retry_attempts",
    this->conn_retry_attempts_, int)

  GET_CONFIG_VALUE (cf, trans_sect, "passive_reconnect_duration",
    this->passive_reconnect_duration_, int)

    GET_CONFIG_VALUE (cf, trans_sect, "passive_connect_duration",
          this->passive_connect_duration_, unsigned int)

  GET_CONFIG_VALUE (cf, trans_sect, "max_output_pause_period",
    this->max_output_pause_period_, int)

  return 0;
}
