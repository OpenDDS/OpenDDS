// -*- C++ -*-
//
// $Id$

#include  "DCPS/DdsDcps_pch.h"
#include  "SimpleTcpSynchResource.h"


#if !defined (__ACE_INLINE__)
#include "SimpleTcpSynchResource.inl"
#endif /* __ACE_INLINE__ */

TAO::DCPS::SimpleTcpSynchResource::~SimpleTcpSynchResource()
{
  DBG_ENTRY("SimpleTcpSynchResource","~SimpleTcpSynchResource");
}


void
TAO::DCPS::SimpleTcpSynchResource::wait_to_unclog()
{
  DBG_ENTRY("SimpleTcpSynchResource","wait_to_unclog");

  // Wait for the blocking to subside.
  if (ACE::handle_write_ready(this->handle_, 0) == -1)
    {
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) ERROR: ACE::handle_write_ready return -1 while waiting "
                 " to unclog.\n"));
    }
}

