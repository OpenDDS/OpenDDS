/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

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
OpenDDS::DCPS::SimpleTcpConfiguration::load(const TransportIdType& id,
                                            ACE_Configuration_Heap& cf)
{
  // The default transport can not be configured by user.
  if (id == DEFAULT_SIMPLE_TCP_ID) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) You can not configure the default SimpleTcp transport(id=%u) !!!\n"),
               id));
    return -1;
  }

  TransportConfiguration::load(id, cf);

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
OpenDDS::DCPS::SimpleTcpConfiguration::dump()
{
  // Acquire lock on the log so the entire dump is output as a block
  // (at least for each process).
  ACE_Log_Msg::instance()->acquire();

  TransportConfiguration::dump();

  ACE_DEBUG((LM_DEBUG,
             ACE_TEXT("(%P|%t) SimpleTcpConfiguration::dump() - ")
             ACE_TEXT("local_address: %C.\n"),
             this->local_address_str_.c_str()));
  ACE_DEBUG((LM_DEBUG,
             ACE_TEXT("(%P|%t) SimpleTcpConfiguration::dump() - ")
             ACE_TEXT("enable_nagle_algorithm: %C.\n"),
             (this->enable_nagle_algorithm_ ? "true" : "false")));
  ACE_DEBUG((LM_DEBUG,
             ACE_TEXT("(%P|%t) SimpleTcpConfiguration::dump() - ")
             ACE_TEXT("conn_retry_initial_delay: %d.\n"),
             this->conn_retry_initial_delay_));
  ACE_DEBUG((LM_DEBUG,
             ACE_TEXT("(%P|%t) SimpleTcpConfiguration::dump() - ")
             ACE_TEXT("conn_retry_backoff_multiplier: %f.\n"),
             this->conn_retry_backoff_multiplier_));
  ACE_DEBUG((LM_DEBUG,
             ACE_TEXT("(%P|%t) SimpleTcpConfiguration::dump() - ")
             ACE_TEXT("conn_retry_attempts: %d.\n"),
            this->conn_retry_attempts_));
  ACE_DEBUG((LM_DEBUG,
             ACE_TEXT("(%P|%t) SimpleTcpConfiguration::dump() - ")
             ACE_TEXT("passive_reconnect_duration: %d.\n"),
             this->passive_reconnect_duration_));
  ACE_DEBUG((LM_DEBUG,
             ACE_TEXT("(%P|%t) SimpleTcpConfiguration::dump() - ")
             ACE_TEXT("passive_connect_duration: %u.\n"),
             this->passive_connect_duration_));
  ACE_DEBUG((LM_DEBUG,
             ACE_TEXT("(%P|%t) SimpleTcpConfiguration::dump() - ")
             ACE_TEXT("max_output_pause_period: %d.\n\n"),
             this->max_output_pause_period_));

  ACE_Log_Msg::instance()->release();
}
