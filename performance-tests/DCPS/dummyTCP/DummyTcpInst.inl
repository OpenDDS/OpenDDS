// -*- C++ -*-
//
// $Id$

#include "dds/DCPS/transport/framework/EntryExit.h"

ACE_INLINE
OpenDDS::DCPS::DummyTcpInst::DummyTcpInst()
  : TransportInst("DummyTcp", "dummy"),
    enable_nagle_algorithm_(false),
    conn_retry_initial_delay_(500),
    conn_retry_backoff_multiplier_(2.0),
    conn_retry_attempts_(3),
    max_output_pause_period_ (-1),
    passive_reconnect_duration_(2000),
    passive_connect_duration_ (10000)
{
  DBG_ENTRY_LVL("DummyTcpInst","DummyTcpInst",5);

  if (local_address_.set (static_cast<unsigned short> (0),
                static_cast<ACE_UINT32> (INADDR_ANY),
                1) != 0)
    ACE_ERROR((LM_ERROR,"(%P|%t) ERROR: "
       "DummyTcpInst::DummyTcpInst could not set default addr\n"));
}
