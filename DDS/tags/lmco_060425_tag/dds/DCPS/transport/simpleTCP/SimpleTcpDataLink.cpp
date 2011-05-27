// -*- C++ -*-
//
// $Id$

#include  "DCPS/DdsDcps_pch.h"
#include  "SimpleTcpDataLink.h"
#include  "SimpleTcpReceiveStrategy.h"

#if !defined (__ACE_INLINE__)
#include "SimpleTcpDataLink.inl"
#endif /* __ACE_INLINE__ */


TAO::DCPS::SimpleTcpDataLink::SimpleTcpDataLink
                                        (const ACE_INET_Addr& remote_address,
                                         TAO::DCPS::SimpleTcpTransport*  transport_impl)
  : DataLink(transport_impl),
    remote_address_(remote_address)
{
  DBG_ENTRY("SimpleTcpDataLink","SimpleTcpDataLink");
  transport_impl->_add_ref ();
  this->transport_ = transport_impl;
}


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

  // Let connection know the datalink for callbacks upon reconnect failure.
  this->connection_->set_datalink (this);

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


/// Associate the new connection object with this datalink object.
/// The states of the "old" connection object are copied to the new 
/// connection object and the "old" connection object is replaced by 
/// the new connection object.
int
TAO::DCPS::SimpleTcpDataLink::reconnect (SimpleTcpConnection* connection)
{
  DBG_ENTRY("SimpleTcpDataLink","reconnect");

  // Sanity check - the connection should exist already since we are reconnecting.
  if (this->connection_.is_nil())
    {
      ACE_ERROR_RETURN((LM_ERROR,
                        "(%P|%t) ERROR: SimpleTcpDataLink::reconnect old connection is nil.\n"),
                       -1);
    }

  // Keep a "copy" of the reference to the connection object for ourselves.
  connection->_add_ref();
  this->connection_->transfer (connection);
  this->connection_ = connection;

  SimpleTcpReceiveStrategy* rs 
    = dynamic_cast <SimpleTcpReceiveStrategy*> (this->receive_strategy_.in ());

  if (rs == 0)
    {
      ACE_ERROR_RETURN((LM_ERROR,
        "(%P|%t) ERROR: SimpleTcpDataLink::reconnect dynamic_cast failed\n"),
        -1);
    }
  // Associate the new connection object with the receiveing strategy and disassociate
  // the old connection object with the receiveing strategy.
  return rs->reset (this->connection_.in ());
}







