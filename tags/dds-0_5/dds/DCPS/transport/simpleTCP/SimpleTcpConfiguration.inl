// -*- C++ -*-
//
// $Id$

#include  "dds/DCPS/transport/framework/EntryExit.h"

ACE_INLINE
TAO::DCPS::SimpleTcpConfiguration::SimpleTcpConfiguration()
{
  DBG_ENTRY("SimpleTcpConfiguration","SimpleTcpConfiguration");

  if (local_address_.set (static_cast<unsigned short> (0),
                static_cast<ACE_UINT32> (INADDR_ANY),
                1) != 0)
    ACE_ERROR((LM_ERROR,"(%P|%t) ERROR: "
       "SimpleTcpConfiguration::SimpleTcpConfiguration could not set default addr\n"));
}

