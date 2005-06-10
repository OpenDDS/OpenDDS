// -*- C++ -*-
//
// $Id$

#include  "DCPS/DdsDcps_pch.h"
#include  "SimpleUdpDataLink.h"


#if !defined (__ACE_INLINE__)
#include "SimpleUdpDataLink.inl"
#endif /* __ACE_INLINE__ */


TAO::DCPS::SimpleUdpDataLink::~SimpleUdpDataLink()
{
  DBG_ENTRY("SimpleUdpDataLink","~SimpleUdpDataLink");
}



void
TAO::DCPS::SimpleUdpDataLink::stop_i()
{
  DBG_ENTRY("SimpleUdpDataLink","stop_i");

  // Nothing to do here.
}

