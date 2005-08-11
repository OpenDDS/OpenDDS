// -*- C++ -*-
//
// $Id$
#include  "DCPS/DdsDcps_pch.h"
#include  "ThreadSynch.h"


#if !defined (__ACE_INLINE__)
#include "ThreadSynch.inl"
#endif /* __ACE_INLINE__ */


TAO::DCPS::ThreadSynch::~ThreadSynch()
{
  DBG_ENTRY("ThreadSynch","~ThreadSynch");
  delete this->resource_;
}


int
TAO::DCPS::ThreadSynch::register_worker_i()
{
  DBG_ENTRY("ThreadSynch","register_worker_i");
  // Default implementation is to do nothing here.  Subclass may override.
  return 0;
}


void
TAO::DCPS::ThreadSynch::unregister_worker_i()
{
  DBG_ENTRY("ThreadSynch","unregister_worker_i");
  // Default implementation is to do nothing here.  Subclass may override.
}

