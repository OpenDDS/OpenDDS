// -*- C++ -*-
//
// $Id$

#include  "SimpleTcpSendStrategy.h"
#include  "SimpleTcpReceiveStrategy.h"
#include  "SimpleTcpConfiguration.h"
#include  "SimpleTcpDataLink.h"
#include  "SimpleTcpSynchResource.h"
#include  "SimpleTcpConnection.h"
#include  "dds/DCPS/transport/framework/NetworkAddress.h"
#include  "dds/DCPS/transport/framework/TransportReactorTask.h"
#include  "dds/DCPS/transport/framework/EntryExit.h"

ACE_INLINE
TAO::DCPS::SimpleTcpTransport::SimpleTcpTransport()
  : acceptor_(this),
    connections_updated_(this->connections_lock_)
{
  DBG_ENTRY("SimpleTcpTransport","SimpleTcpTransport");
}


/// This method is called by a SimpleTcpConnection object that has been
/// created and opened by our acceptor_ as a result of passively
/// accepting a connection on our local address.  The connection object
/// is "giving itself away" for us to manage.  Ultimately, the connection
/// object needs to be paired with a DataLink object that is (or will be)
/// expecting this passive connection to be established.
ACE_INLINE void
TAO::DCPS::SimpleTcpTransport::passive_connection
                                        (const ACE_INET_Addr& remote_address,
                                         SimpleTcpConnection* connection)
{
  DBG_ENTRY("SimpleTcpTransport","passive_connection");

  // Take ownership of the passed-in connection pointer.
  SimpleTcpConnection_rch connection_obj = connection;

  GuardType guard(this->connections_lock_);

  if (this->connections_.bind(remote_address,connection_obj) != 0)
    {
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) ERROR: Unable to bind SimpleTcpConnection object "
                 "to the connections_ map.\n"));
    }

  // Regardless of the outcome of the bind operation, let's tell any threads
  // that are wait()'ing on the connections_updated_ condition to check
  // the connections_ map again.
  this->connections_updated_.broadcast();
}


/// Actively establish a connection to the remote address.
ACE_INLINE int
TAO::DCPS::SimpleTcpTransport::make_active_connection
                                        (const ACE_INET_Addr& remote_address,
                                         SimpleTcpDataLink*   link)
{
  DBG_ENTRY("SimpleTcpTransport","make_active_connection");

  // Create the connection object here.
  SimpleTcpConnection_rch connection = new SimpleTcpConnection();

  // Ask the connection object to attempt the active connection establishment.
  if (connection->active_establishment(remote_address,
                                       this->tcp_config_->local_address_,
                                       this->tcp_config_.in()) != 0)
    {
      return -1;
    }

  return this->connect_datalink(link, connection.in());
}


ACE_INLINE int
TAO::DCPS::SimpleTcpTransport::make_passive_connection
                                        (const ACE_INET_Addr& remote_address,
                                         SimpleTcpDataLink*   link)
{
  DBG_ENTRY("SimpleTcpTransport","make_passive_connection");

  SimpleTcpConnection_rch connection;

  // Look in our connections_ map to see if the passive connection
  // has already been established for the remote_address.  If so, we
  // will extract it from the connections_ map and give it to the link.
  {
    GuardType guard(this->connections_lock_);

    while (this->connections_.unbind(remote_address,connection) != 0)
      {
        // There is no connection object waiting for us (yet).
        // We need to wait on the connections_updated_ condition
        // and then re-attempt to extract our connection.
        this->connections_updated_.wait();

        // TBD SOON - Check to see if we we woke up because the Transport
        //            is shutting down.  If so, return a -1 now.
      }
  }

  return this->connect_datalink(link, connection.in());
}


/// Common code used by make_active_connection() and make_passive_connection().
ACE_INLINE int
TAO::DCPS::SimpleTcpTransport::connect_datalink
                                        (SimpleTcpDataLink*   link,
                                         SimpleTcpConnection* connection)
{
  DBG_ENTRY("SimpleTcpTransport","connect_datalink");

  TransportSendStrategy_rch send_strategy = 
             new SimpleTcpSendStrategy(this->tcp_config_.in(),
                                       connection,
                                       new SimpleTcpSynchResource(connection));

  TransportReceiveStrategy_rch receive_strategy = 
                       new SimpleTcpReceiveStrategy(link,
                                                    connection,
                                                    this->reactor_task_.in());

  if (link->connect(connection,
                    send_strategy.in(),
                    receive_strategy.in()) != 0)
    {
      return -1;
    }

  return 0;
}

