// -*- C++ -*-
//
// $Id$

#include "ReliableMulticast_pch.h"
#include "ReliableMulticastTransportConfiguration.h"

#if !defined (__ACE_INLINE__)
#include "ReliableMulticastTransportConfiguration.inl"
#endif /* __ACE_INLINE__ */

int
TAO::DCPS::ReliableMulticastTransportConfiguration::load(
  const TransportIdType& id,
  ACE_Configuration_Heap& config
  )
{
  if (id == DEFAULT_RELIABLE_MULTICAST_PUB_ID || id == DEFAULT_RELIABLE_MULTICAST_SUB_ID)
  {
    ACE_ERROR_RETURN(
      (LM_ERROR, "(%P|%t) ERROR: You can not configure the default reliable multicast transport (id=%u).\n", id),
      -1
      );
  }

  int result = TransportConfiguration::load(id, config);
  if (result == 0)
  {
    ACE_Configuration_Section_Key trans_sect;
    ACE_CString str;

    GET_CONFIG_STRING_VALUE(config, trans_sect, "local_address", str);
    if (str != "")
    {
      local_address_.set(str.c_str());
    }
    else
    {
      ACE_ERROR_RETURN(
        (LM_ERROR, "(%P|%t) ERROR: No local_address configuration value specified.\n"),
        -1
        );
    }

    GET_CONFIG_STRING_VALUE(config, trans_sect, "multicast_group_address", str);
    if (str != "")
    {
      multicast_group_address_.set(str.c_str());
    }
    else
    {
      ACE_ERROR_RETURN(
        (LM_ERROR, "(%P|%t) ERROR: No multicast_group_address configuration value specified.\n"),
        -1
        );
    }

    GET_CONFIG_VALUE(config, trans_sect, "receiver", receiver_, bool);

    GET_CONFIG_VALUE(config, trans_sect, "sender_history_size", sender_history_size_, size_t);

    GET_CONFIG_VALUE(config, trans_sect, "receiver_buffer_size", receiver_buffer_size_, size_t);
  }
  return result;
}
