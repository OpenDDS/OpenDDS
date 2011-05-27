// -*- C++ -*-
//
// $Id$
#include  "ace/Reactor.h"
#include  "EntryExit.h"

ACE_INLINE ACE_Reactor*
TAO::DCPS::TransportReactorTask::get_reactor()
{
  DBG_SUB_ENTRY("TransportReactorTask","get_reactor",1);
  return this->reactor_;
}


ACE_INLINE const ACE_Reactor*
TAO::DCPS::TransportReactorTask::get_reactor() const
{
  DBG_SUB_ENTRY("TransportReactorTask","get_reactor",2);
  return this->reactor_;
}

