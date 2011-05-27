// -*- C++ -*-
//
// $Id$

#include  "SimpleMcast_pch.h"
#include  "SimpleMcastDataLink.h"


#if !defined (__ACE_INLINE__)
#include "SimpleMcastDataLink.inl"
#endif /* __ACE_INLINE__ */


TAO::DCPS::SimpleMcastDataLink::~SimpleMcastDataLink()
{
  DBG_ENTRY_LVL("SimpleMcastDataLink","~SimpleMcastDataLink",5);
}



void
TAO::DCPS::SimpleMcastDataLink::stop_i()
{
  DBG_ENTRY_LVL("SimpleMcastDataLink","stop_i",5);

  // Nothing to do here.
}

