// -*- C++ -*-
//
// $Id$

#include "ReliableMulticast_pch.h"
#include "ReliableMulticastTransportConfiguration.h"

#if !defined (__ACE_INLINE__)
#include "ReliableMulticastTransportConfiguration.inl"
#endif /* __ACE_INLINE__ */

int
OpenDDS::DCPS::ReliableMulticastTransportConfiguration::load(
  const TransportIdType& id,
  ACE_Configuration_Heap& config
  )
{
  if (id == DEFAULT_RELIABLE_MULTICAST_PUB_ID || id == DEFAULT_RELIABLE_MULTICAST_SUB_ID)
  {
    ACE_ERROR_RETURN(
      (LM_ERROR, ACE_TEXT("(%P|%t) ERROR: You can not configure the default reliable multicast transport (id=%u).\n"), id),
      -1
      );
  }

  int result = TransportConfiguration::load(id, config);
  if (result == 0)
  {
    ACE_TCHAR section [50];
    ACE_OS::sprintf (section, ACE_TEXT("%s%d")
                     , ACE_TEXT_ALWAYS_CHAR(TRANSPORT_SECTION_NAME_PREFIX), id);
    const ACE_Configuration_Section_Key &root = config.root_section ();
    ACE_Configuration_Section_Key trans_sect;
    if (config.open_section (root, section, 0, trans_sect) != 0) {
      ACE_ERROR_RETURN ((LM_ERROR,
                         ACE_TEXT ("Failed to open section: section %s\n"), section),
                        -1);
    }

    ACE_TString str;
    GET_CONFIG_STRING_VALUE(config, trans_sect, ACE_TEXT("local_address"), str);
    if (str != ACE_TEXT(""))
    {
      this->local_address_str_ = str.c_str ();
      this->local_address_.set (this->local_address_str_.c_str ());
    }

    GET_CONFIG_STRING_VALUE(config, trans_sect, ACE_TEXT("multicast_group_address"), str);
    if (str != ACE_TEXT(""))
    {
      this->multicast_group_address_str_ = str.c_str ();
      this->multicast_group_address_.set (multicast_group_address_str_.c_str ());
    }

    GET_CONFIG_VALUE(config, trans_sect, ACE_TEXT("receiver"), receiver_, bool);

    GET_CONFIG_VALUE(config, trans_sect, ACE_TEXT("sender_history_size"), sender_history_size_, size_t);

    GET_CONFIG_VALUE(config, trans_sect, ACE_TEXT("receiver_buffer_size"), receiver_buffer_size_, size_t);
  }
  return result;
}

OpenDDS::DCPS::ReliableMulticastTransportConfiguration::~ReliableMulticastTransportConfiguration()
{
}
