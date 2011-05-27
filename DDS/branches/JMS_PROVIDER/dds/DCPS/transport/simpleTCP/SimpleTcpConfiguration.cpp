// -*- C++ -*-
//
// $Id$
#include "SimpleTcp_pch.h"
#include "SimpleTcpConfiguration.h"


#if !defined (__ACE_INLINE__)
#include "SimpleTcpConfiguration.inl"
#endif /* __ACE_INLINE__ */


OpenDDS::DCPS::SimpleTcpConfiguration::~SimpleTcpConfiguration()
{
  DBG_ENTRY_LVL("SimpleTcpConfiguration","~SimpleTcpConfiguration",6);
}

int
OpenDDS::DCPS::SimpleTcpConfiguration::load (const TransportIdType& id,
                                         ACE_Configuration_Heap& cf)
{
  // The default transport can not be configured by user.
  if (id == DEFAULT_SIMPLE_TCP_ID)
    {
      ACE_ERROR ((LM_ERROR, ACE_TEXT("(%P|%t)You can not configure the default SimpleTcp transport(id=%u) !!! \n"),
        id));
      return -1;
    }

  TransportConfiguration::load (id, cf);

  ACE_TCHAR section [50];
  ACE_OS::sprintf (section, ACE_TEXT("%s%d")
                   , ACE_TEXT_ALWAYS_CHAR(TRANSPORT_SECTION_NAME_PREFIX), id);
  const ACE_Configuration_Section_Key &root = cf.root_section ();
  ACE_Configuration_Section_Key trans_sect;
  if (cf.open_section (root, section, 0, trans_sect) != 0)
    ACE_ERROR_RETURN ((LM_ERROR,
                       ACE_TEXT ("Failed to open section \"%s\" \n"), section),
                       -1);

  ACE_TString local_address;
  GET_CONFIG_STRING_VALUE (cf, trans_sect, ACE_TEXT("local_address"), local_address);
  if (local_address != ACE_TEXT(""))
  {
    this->local_address_str_ = local_address.c_str ();
    this->local_address_.set (local_address_str_.c_str ());
  }

  GET_CONFIG_VALUE (cf, trans_sect, ACE_TEXT("enable_nagle_algorithm"),
    this->enable_nagle_algorithm_, bool)

  GET_CONFIG_VALUE (cf, trans_sect, ACE_TEXT("conn_retry_initial_delay"),
    this->conn_retry_initial_delay_, int)

    GET_CONFIG_VALUE (cf, trans_sect, ACE_TEXT("conn_retry_backoff_multiplier"),
                      this->conn_retry_backoff_multiplier_, double)

  GET_CONFIG_VALUE (cf, trans_sect, ACE_TEXT("conn_retry_attempts"),
    this->conn_retry_attempts_, int)

  GET_CONFIG_VALUE (cf, trans_sect, ACE_TEXT("passive_reconnect_duration"),
    this->passive_reconnect_duration_, int)

    GET_CONFIG_VALUE (cf, trans_sect, ACE_TEXT("passive_connect_duration"),
          this->passive_connect_duration_, unsigned int)

  GET_CONFIG_VALUE (cf, trans_sect, ACE_TEXT("max_output_pause_period"),
    this->max_output_pause_period_, int)

  return 0;
}
