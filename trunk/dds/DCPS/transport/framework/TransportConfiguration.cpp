// -*- C++ -*-
//
// $Id$

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "TransportConfiguration.h"
#include "ThreadSynchStrategy.h"
#include "EntryExit.h"

#if !defined (__ACE_INLINE__)
# include "TransportConfiguration.inl"
#endif /* ! __ACE_INLINE__ */


OpenDDS::DCPS::TransportConfiguration::~TransportConfiguration()
{
  DBG_ENTRY_LVL("TransportConfiguration","~TransportConfiguration",5);
  delete this->send_thread_strategy_;
}

int
OpenDDS::DCPS::TransportConfiguration::load (const TransportIdType& id, ACE_Configuration_Heap& cf)
{
  char section [50];
  ACE_OS::sprintf (section, "%s%u", TRANSPORT_SECTION_NAME_PREFIX, id);
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
  GET_CONFIG_VALUE (cf, sect, "thread_per_connection", this->thread_per_connection_, bool)
  GET_CONFIG_VALUE (cf, sect, "keep_link", this->keep_link_, bool)

  adjust_config_value ();
  return 0;
}



