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


/// The SimpleTcpTransport calls this method when it has an established
/// connection object for us.  This call puts this SimpleTcpDataLink into
/// the "connected" state.
int
TAO::DCPS::SimpleTcpDataLink::connect
                                 (SimpleTcpConnection*      connection,
                                  TransportSendStrategy*    send_strategy,
                                  TransportReceiveStrategy* receive_strategy)
{
  DBG_ENTRY("SimpleTcpDataLink","connect");

  // Sanity check - cannot connect() if we are already connected.
  if (!this->connection_.is_nil())
    {
      ACE_ERROR_RETURN((LM_ERROR,
                        "(%P|%t) ERROR: SimpleTcpDataLink already connected.\n"),
                       -1);
    }

  // Keep a "copy" of the reference to the connection object for ourselves.
  connection->_add_ref();
  this->connection_ = connection;

  // And lastly, inform our base class (DataLink) that we are now "connected",
  // and it should start the strategy objects.
  if (this->start(send_strategy,receive_strategy) != 0)
    {
      // Our base (DataLink) class failed to start the strategy objects.
      // We need to "undo" some things here before we return -1 to indicate
      // that an error has taken place.

      // Drop our reference to the connection object.
      this->connection_ = 0;

      return -1;
    }

  return 0;
}

