// -*- C++ -*-
//
// $Id$

#include  "dds/DCPS/transport/framework/EntryExit.h"

ACE_INLINE
TAO::DCPS::SimpleTcpConfiguration::SimpleTcpConfiguration()
  : enable_nagle_algorithm_(false),
    attempt_connection_reestablishment_(true),
    conn_retry_initial_delay_(500),
    conn_retry_backoff_multiplier_(2),
    conn_retry_attempts_(3),
    passive_reconenct_duration_(2000)
{
  DBG_ENTRY("SimpleTcpConfiguration","SimpleTcpConfiguration");

  transport_type_ = "SimpleTcp";

  if (local_address_.set (static_cast<unsigned short> (0),
                static_cast<ACE_UINT32> (INADDR_ANY),
                1) != 0)
    ACE_ERROR((LM_ERROR,"(%P|%t) ERROR: "
       "SimpleTcpConfiguration::SimpleTcpConfiguration could not set default addr\n"));
}

