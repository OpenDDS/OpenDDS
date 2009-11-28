/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "TransportConfiguration.h"
#include "ThreadSynchStrategy.h"
#include "EntryExit.h"

#include <sstream>

#if !defined (__ACE_INLINE__)
# include "TransportConfiguration.inl"
#endif /* !__ACE_INLINE__ */

OpenDDS::DCPS::TransportConfiguration::~TransportConfiguration()
{
  DBG_ENTRY_LVL("TransportConfiguration","~TransportConfiguration",6);
  delete this->send_thread_strategy_;
}

ACE_TString
OpenDDS::DCPS::TransportConfiguration::id_to_section_name(const TransportIdType& id)
{
  std::basic_ostringstream<ACE_TCHAR> oss;
  oss << TRANSPORT_SECTION_NAME_PREFIX << id;
  return oss.str().c_str();
}

int
OpenDDS::DCPS::TransportConfiguration::load(const TransportIdType& id
                                            , ACE_Configuration_Heap& cf)
{
  ACE_TString sect_name = id_to_section_name(id);
  const ACE_Configuration_Section_Key &root = cf.root_section();
  ACE_Configuration_Section_Key sect;

  if (cf.open_section(root, sect_name.c_str(), 0, sect) != 0)
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("Failed to open section: %s\n"),
                      sect_name.c_str()), -1);

  GET_CONFIG_VALUE(cf, sect, ACE_TEXT("swap_bytes"), this->swap_bytes_, bool)
  GET_CONFIG_VALUE(cf, sect, ACE_TEXT("queue_messages_per_pool"), this->queue_messages_per_pool_, size_t)
  GET_CONFIG_VALUE(cf, sect, ACE_TEXT("queue_initial_pools"), this->queue_initial_pools_, size_t)
  GET_CONFIG_VALUE(cf, sect, ACE_TEXT("max_packet_size"), this->max_packet_size_, ACE_UINT32)
  GET_CONFIG_VALUE(cf, sect, ACE_TEXT("max_samples_per_packet"), this->max_samples_per_packet_, size_t)
  GET_CONFIG_VALUE(cf, sect, ACE_TEXT("optimum_packet_size"), this->optimum_packet_size_, ACE_UINT32)
  GET_CONFIG_STRING_VALUE(cf, sect, ACE_TEXT("transport_type"), this->transport_type_)
  GET_CONFIG_VALUE(cf, sect, ACE_TEXT("thread_per_connection"), this->thread_per_connection_, bool)
  GET_CONFIG_VALUE(cf, sect, ACE_TEXT("datalink_release_delay"), this->datalink_release_delay_, int)
  GET_CONFIG_VALUE(cf, sect, ACE_TEXT("datalink_control_chunks"), this->datalink_control_chunks_, size_t)

  adjust_config_value();
  return 0;
}
