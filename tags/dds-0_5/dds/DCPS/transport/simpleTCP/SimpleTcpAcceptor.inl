// -*- C++ -*-
//
// $Id$

#include  "SimpleTcpTransport.h"
#include  "dds/DCPS/transport/framework/EntryExit.h"

ACE_INLINE TAO::DCPS::SimpleTcpTransport*
TAO::DCPS::SimpleTcpAcceptor::transport()
{
  DBG_ENTRY("SimpleTcpAcceptor","transport");
  // Return a new reference to the caller (the caller is responsible for
  // the reference).
  SimpleTcpTransport_rch tmp = this->transport_;
  return tmp._retn();
}


ACE_INLINE void
TAO::DCPS::SimpleTcpAcceptor::transport_shutdown()
{
  DBG_ENTRY("SimpleTcpAcceptor","transport_shutdown");

  // Drop the reference to the SimpleTcpTransport object.
  this->transport_ = 0;
}

