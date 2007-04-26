// -*- C++ -*-
//
// $Id$
#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "ThreadSynchResource.h"


#if !defined (__ACE_INLINE__)
#include "ThreadSynchResource.inl"
#endif /* __ACE_INLINE__ */


TAO::DCPS::ThreadSynchResource::~ThreadSynchResource()
{
  DBG_ENTRY_LVL("ThreadSynchResource","~ThreadSynchResource",5);
  delete this->timeout_;
}

