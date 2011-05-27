// -*- C++ -*-
//
// $Id$

#include  "DCPS/DdsDcps_pch.h"
#include  "TransportConfiguration.h"
#include  "ThreadSynchStrategy.h"
#include  "EntryExit.h"

#if !defined (__ACE_INLINE__)
# include "TransportConfiguration.inl"
#endif /* ! __ACE_INLINE__ */


TAO::DCPS::TransportConfiguration::~TransportConfiguration()
{
  DBG_ENTRY("TransportConfiguration","~TransportConfiguration");
  delete this->send_thread_strategy_;
}
