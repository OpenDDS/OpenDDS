// -*- C++ -*-
//
// $Id$

#include "dds/DCPS/transport/framework/EntryExit.h"

ACE_INLINE
OpenDDS::DCPS::SimpleTcpConfiguration::SimpleTcpConfiguration()
  : enable_nagle_algorithm_(false),
    conn_retry_initial_delay_(500),
    conn_retry_backoff_multiplier_(2.0),
    conn_retry_attempts_(3),
    max_output_pause_period_ (-1),
    passive_reconnect_duration_(2000),
    passive_connect_duration_ (10000)
{
  DBG_ENTRY_LVL("SimpleTcpConfiguration","SimpleTcpConfiguration",6);

  transport_type_ = ACE_TEXT("SimpleTcp");
}
