// -*- C++ -*-
//
// $Id$

#include  "SimpleUdp_pch.h"
#include  "SimpleUdpDataLink.h"


#if !defined (__ACE_INLINE__)
#include "SimpleUdpDataLink.inl"
#endif /* __ACE_INLINE__ */


TAO::DCPS::SimpleUdpDataLink::~SimpleUdpDataLink()
{
  DBG_ENTRY_LVL("SimpleUdpDataLink","~SimpleUdpDataLink",5);
}



void
TAO::DCPS::SimpleUdpDataLink::stop_i()
{
  DBG_ENTRY_LVL("SimpleUdpDataLink","stop_i",5);

  // Nothing to do here.
}

