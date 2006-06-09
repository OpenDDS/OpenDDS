// -*- C++ -*-
//
// $Id$
#include  "SimpleUdp_pch.h"
#include  "SimpleUdpConfiguration.h"


#if !defined (__ACE_INLINE__)
#include "SimpleUdpConfiguration.inl"
#endif /* __ACE_INLINE__ */


TAO::DCPS::SimpleUdpConfiguration::~SimpleUdpConfiguration()
{
  DBG_ENTRY("SimpleUdpConfiguration","~SimpleUdpConfiguration");
}

int 
TAO::DCPS::SimpleUdpConfiguration::load (const TransportIdType& id, 
                                         ACE_Configuration_Heap& cf)
{
  // Call the base class method through 'this' to help VC6 figure out
  // what to do.
  this->TransportConfiguration::load (id, cf);
  
  char section [20];
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

  return 0;
}

