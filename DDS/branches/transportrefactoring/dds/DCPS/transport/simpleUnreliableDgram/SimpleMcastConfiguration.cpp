// -*- C++ -*-
//
// $Id$
#include "SimpleUnreliableDgram_pch.h"
#include "SimpleMcastConfiguration.h"


#if !defined (__ACE_INLINE__)
#include "SimpleMcastConfiguration.inl"
#endif /* __ACE_INLINE__ */


OpenDDS::DCPS::SimpleMcastConfiguration::~SimpleMcastConfiguration()
{
  DBG_ENTRY_LVL("SimpleMcastConfiguration","~SimpleMcastConfiguration",5);
}

int
OpenDDS::DCPS::SimpleMcastConfiguration::load (const TransportIdType& id,
                                         ACE_Configuration_Heap& cf)
{
  // The default transport can not be configured by user.
  if (id == DEFAULT_SIMPLE_MCAST_PUB_ID || id == DEFAULT_SIMPLE_MCAST_SUB_ID)
    {
       ACE_ERROR ((LM_ERROR, "(%P|%t)You can not configure the default SimpleMcast transport(id=%u) !!! \n",
         id));
       return -1;
    }

  // Call the base class method through 'this' to help VC6 figure out
  // what to do.
  this->TransportConfiguration::load (id, cf);

  char section [50];
  ACE_OS::sprintf (section, "%s%d", TRANSPORT_SECTION_NAME_PREFIX, id);
  const ACE_Configuration_Section_Key &root = cf.root_section ();
  ACE_Configuration_Section_Key trans_sect;
  if (cf.open_section (root, section, 0, trans_sect) != 0)
    ACE_ERROR_RETURN ((LM_ERROR,
                       ACE_TEXT ("Failed to open section: section %s\n"), section),
                       -1);

  ACE_CString local_address;
  GET_CONFIG_STRING_VALUE (cf, trans_sect, "local_address", local_address);
  if (local_address != "")
  {
    this->local_address_.set (local_address.c_str ());
  }

  ACE_CString multicast_group_address;
  GET_CONFIG_STRING_VALUE (cf, trans_sect, "multicast_group_address", multicast_group_address);
  if (multicast_group_address != "")
  {
    this->multicast_group_address_.set (multicast_group_address.c_str ());
  }

  GET_CONFIG_VALUE (cf, trans_sect, "receiver", this->receiver_, bool);

  GET_CONFIG_VALUE (cf, trans_sect, "max_output_pause_period",
    this->max_output_pause_period_, int);

  return 0;
}

