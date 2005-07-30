// -*- C++ -*-
//
// $Id$

#include  "SimpleTcpTransport.h"
#include  "SimpleTcpConnection.h"
#include  "dds/DCPS/transport/framework/TransportSendStrategy.h"
#include  "dds/DCPS/transport/framework/TransportReceiveStrategy.h"
#include  "dds/DCPS/transport/framework/EntryExit.h"

ACE_INLINE
TAO::DCPS::SimpleTcpDataLink::SimpleTcpDataLink
                                        (const ACE_INET_Addr& remote_address,
                                         SimpleTcpTransport*  transport_impl)
  : DataLink(transport_impl),
    remote_address_(remote_address)
{
  DBG_ENTRY("SimpleTcpDataLink","SimpleTcpDataLink");
}



ACE_INLINE const ACE_INET_Addr&
TAO::DCPS::SimpleTcpDataLink::remote_address() const
{
  DBG_ENTRY("SimpleTcpDataLink","remote_address");
  return this->remote_address_;
}


/// The SimpleTcpTransport calls this method when it has an established
/// connection object for us.  This call puts this SimpleTcpDataLink into
/// the "connected" state.
ACE_INLINE int
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

