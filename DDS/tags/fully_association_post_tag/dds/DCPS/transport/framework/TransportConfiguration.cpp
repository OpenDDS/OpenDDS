// -*- C++ -*-
//
// $Id$

#include  "DCPS/DdsDcps_pch.h"
#include  "TransportConfiguration.h"
#include  "ThreadSynchStrategy.h"
#include  "EntryExit.h"

#if !defined (__ACE_INLINE__)
# include "TransportConfiguration.inl"
#endif /* ! __ACE_INLINE__ */


TAO::DCPS::TransportConfiguration::~TransportConfiguration()
{
  DBG_ENTRY("TransportConfiguration","~TransportConfiguration");
  delete this->send_thread_strategy_;
}

int 
TAO::DCPS::TransportConfiguration::load (const TransportIdType& id, ACE_Configuration_Heap& cf)
{
  char section [20];
  ACE_OS::sprintf (section, "%s%d", TRANSPORT_SECTION_NAME_PREFIX, id);
  const ACE_Configuration_Section_Key &root = cf.root_section ();
  ACE_Configuration_Section_Key sect;
  if (cf.open_section (root, section, 0, sect) != 0)
    ACE_ERROR_RETURN ((LM_ERROR, 
                       ACE_TEXT ("Failed to open section: %s\n"), section),
                       -1);
  GET_CONFIG_VALUE (cf, sect, "swap_bytes", this->swap_bytes_, bool)
  GET_CONFIG_VALUE (cf, sect, "queue_messages_per_pool", this->queue_messages_per_pool_, size_t)
  GET_CONFIG_VALUE (cf, sect, "queue_initial_pools", this->queue_initial_pools_, size_t)
  GET_CONFIG_VALUE (cf, sect, "max_packet_size", this->max_packet_size_, ACE_UINT32)
  GET_CONFIG_VALUE (cf, sect, "max_samples_per_packet", this->max_samples_per_packet_, size_t)
  GET_CONFIG_VALUE (cf, sect, "optimum_packet_size", this->optimum_packet_size_, ACE_UINT32)
  GET_CONFIG_STRING_VALUE (cf, sect, "transport_type", this->transport_type_)
  
  adjust_config_value ();
  return 0;
}



