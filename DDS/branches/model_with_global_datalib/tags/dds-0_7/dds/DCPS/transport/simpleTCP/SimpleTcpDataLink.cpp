// -*- C++ -*-
//
// $Id$

#include  "DCPS/DdsDcps_pch.h"
#include  "SimpleTcpDataLink.h"


#if !defined (__ACE_INLINE__)
#include "SimpleTcpDataLink.inl"
#endif /* __ACE_INLINE__ */


TAO::DCPS::SimpleTcpDataLink::~SimpleTcpDataLink()
{
  DBG_ENTRY("SimpleTcpDataLink","~SimpleTcpDataLink");
}


/// Called when the DataLink has been "stopped" for some reason.  It could
/// be called from the DataLink::transport_shutdown() method (when the
/// TransportImpl is handling a shutdown() call).  Or, it could be called
/// from the DataLink::release_reservations() method, when it discovers that
/// it has just released the last remaining reservations from the DataLink,
/// and the DataLink is in the process of "releasing" itself.
void
TAO::DCPS::SimpleTcpDataLink::stop_i()
{
  DBG_ENTRY("SimpleTcpDataLink","stop_i");

  if (!this->connection_.is_nil())
    {
      // Tell the connection object to disconnect.
      this->connection_->disconnect();

      // Drop our reference to the connection object.
      this->connection_ = 0;
    }
}

