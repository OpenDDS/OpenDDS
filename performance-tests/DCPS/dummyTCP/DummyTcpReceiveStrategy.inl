// -*- C++ -*-
//
// $Id$

#include "dds/DCPS/transport/framework/TransportReactorTask.h"
#include "dds/DCPS/transport/framework/EntryExit.h"

ACE_INLINE ACE_Reactor*
OpenDDS::DCPS::DummyTcpReceiveStrategy::get_reactor()
{
  DBG_ENTRY_LVL("DummyTcpReceiveStrategy","get_reactor",5);
  return this->reactor_task_->get_reactor ();
}


ACE_INLINE bool
OpenDDS::DCPS::DummyTcpReceiveStrategy::gracefully_disconnected ()
{
 return this->gracefully_disconnected_;
}

