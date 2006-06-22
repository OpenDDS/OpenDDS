// -*- C++ -*-
//
// $Id$

#include  "DCPS/DdsDcps_pch.h"
#include  "SimpleTcpTransport.h"

#if !defined (__ACE_INLINE__)
#include "SimpleTcpTransport.inl"
#endif /* __ACE_INLINE__ */


TAO::DCPS::SimpleTcpTransport::~SimpleTcpTransport()
{
  DBG_ENTRY("SimpleTcpTransport","~SimpleTcpTransport");
}


/// This is called from the base class (TransportImpl) as a result of
/// and add_publications() or add_subscriptions() call on a
/// TransportInterface object.  The TransportInterface object calls
/// reserve_datalink() on the TransportImpl, and the TransportImpl calls
/// find_or_create_datalink() on us (a concrete subclass of TransportImpl).
///
/// The connect_as_publisher will be set to 0 if this method was called
/// due to an add_publications() call on a TransportInterface object.
/// This means false (0).  It is *not* connecting as a publisher.
///
/// The connect_as_publisher will be set to 1 if this method was called
/// due to an add_subscriptions() call on a TransportInterface object.
/// This means true (1).  It *is* connecting as a publisher.
TAO::DCPS::DataLink*
TAO::DCPS::SimpleTcpTransport::find_or_create_datalink
                         (const TransportInterfaceInfo& remote_info,
                          int                           connect_as_publisher)
{
  DBG_ENTRY("SimpleTcpTransport","find_or_create_datalink");

  // Get the remote address from the "blob" in the remote_info struct.
  NetworkAddress* network_order_address =
                          (NetworkAddress*)(remote_info.data.get_buffer());

  ACE_INET_Addr remote_address;
  network_order_address->to_addr(remote_address);

  // First, we have to try to find an existing (connected) DataLink
  // that suits the caller's needs.
  GuardType guard(this->links_lock_);

  SimpleTcpDataLink_rch link;

  if (this->links_.find(remote_address,link) == 0)
    {
      SimpleTcpConnection_rch con = link->get_connection ();
      if (con->is_connector () && ! con->is_connected ())
        {
          bool on_new_association = true;
          if (con->reconnect (on_new_association) == -1)
            {
              ACE_ERROR_RETURN ((LM_ERROR,
                "(%P|%t) ERROR: Unable to reconnect to remote %s:%d.\n", 
                remote_address.get_host_addr (),
                remote_address.get_port_number ()),
                0);
            }
        }
      // This means we found a suitable (and already connected) DataLink.
      // We can return it now since we are done.
      return link._retn();
    }

  // The "find" part of the find_or_create_datalink has been attempted, and
  // we failed to find a suitable DataLink.  This means we need to move on
  // and attempt the "create" part of "find_or_create_datalink".

  // Here is where we actually create the DataLink.
  link = new SimpleTcpDataLink(remote_address, this);

  // Attempt to bind the SimpleTcpDataLink to our links_ map.
  if (this->links_.bind(remote_address,link) != 0)
    {
      // We failed to bind the new DataLink into our links_ map.
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) ERROR: Unable to bind new SimpleTcpDataLink to "
                 "SimpleTcpTransport in links_ map.\n"));

      // On error, we return a NULL pointer.
      return 0;
    }

  // Now we need to attempt to establish a connection for the DataLink.
  int result;

  // Active or passive connection establishment is based upon the value
  // on the connect_as_publisher argument.
  if (connect_as_publisher == 1)
    {
      result = this->make_active_connection(remote_address, link.in());

      if (result != 0)
        {
          ACE_ERROR((LM_ERROR,
                     "(%P|%t) ERROR: Failed to make active connection.\n"));
        }
    }
  else
    {
      result = this->make_passive_connection(remote_address, link.in());

      if (result != 0)
        {
          ACE_ERROR((LM_ERROR,
                     "(%P|%t) ERROR: Failed to make passive connection.\n"));
        }
    }

  if (result != 0)
    {
      // Make sure that we unbind the link (that failed to establish a
      // connection) from our links_ map.  We intentionally ignore the
      // return code from the unbind() call since we know that we just
      // did the bind() moments ago - and with the links_lock_ acquired
      // the whole time.
      this->links_.unbind(remote_address);

      // On error, return a NULL pointer.
      return 0;
    }

  // That worked.  Return a reference to the DataLink that the caller will
  // be responsible for.
  return link._retn();
}


int
TAO::DCPS::SimpleTcpTransport::configure_i(TransportConfiguration* config)
{
  DBG_ENTRY("SimpleTcpTransport","configure_i");

  // Downcast the config argument to a SimpleTcpConfiguration*
  SimpleTcpConfiguration* tcp_config = ACE_static_cast(SimpleTcpConfiguration*,
                                                       config);

  if (tcp_config == 0)
    {
      // The downcast failed.
      ACE_ERROR_RETURN((LM_ERROR,
                        "(%P|%t) ERROR: Failed downcast from TransportConfiguration "
                        "to SimpleTcpConfiguration.\n"),
                       -1);
    }

  // Ask our base class for a "copy" of the reference to the reactor task.
  this->reactor_task_ = reactor_task();

  if (this->reactor_task_.is_nil())
    {
      // It looks like our base class has either been shutdown, or it has
      // erroneously never been supplied with the reactor task.
      ACE_ERROR_RETURN((LM_ERROR,
                        "(%P|%t) ERROR: SimpleTcpTransport requires a reactor in "
                        "order to open its acceptor_.\n"),
                       -1);
    }

  // Make a "copy" of the reference for ourselves.
  tcp_config->_add_ref();
  this->tcp_config_ = tcp_config;

  
  // If the IP address in the INET_Addr is the INADDR_ANY address,
  // then force the actual IP address to be used by initializing a new
  // INET_Addr with the hostname from the original one.  If that fails
  // then something is seriously wrong with the systems networking
  // setup.
  if (tcp_config->local_address_.get_ip_address () == INADDR_ANY)
    {
      ACE_INET_Addr new_addr;
      int result = new_addr.set (
                   tcp_config->local_address_.get_port_number (),
                   tcp_config->local_address_.get_host_name ());

      if (result != 0)
        ACE_ERROR_RETURN((LM_ERROR,
                  "(%P|%t) ERROR: SimpleTcpTransport::configure_i"
                  " could not get host name!!\n"),
                 -1);

      const char *tmp = 0; // just to help debugging
      tmp = new_addr.get_host_addr ();

      this->tcp_config_->local_address_ = new_addr;
    }

  // Open the reconnect task
  if (this->reconnect_task_->open ())
    {
      ACE_ERROR_RETURN((LM_ERROR,
                        "(%P|%t) ERROR: Reconnect task failed to open : %p\n",
                        "open"),
                        -1);
    }

  // Open our acceptor object so that we can accept passive connections
  // on our this->tcp_config_->local_address_.

  if (this->acceptor_.open(this->tcp_config_->local_address_,
                           this->reactor_task_->get_reactor()) != 0)
    {
      // Remember to drop our reference to the tcp_config_ object since
      // we are about to return -1 here, which means we are supposed to
      // keep a copy after all.
      SimpleTcpConfiguration_rch cfg = this->tcp_config_._retn();

      ACE_ERROR_RETURN((LM_ERROR,
                        "(%P|%t) ERROR: Acceptor failed to open %s:%d: %p\n",
                        cfg->local_address_.get_host_addr (), 
                        cfg->local_address_.get_port_number (),
                        "open"),
                       -1);
    }

  // update the port number (incase port zero was given).
  ACE_INET_Addr address;
  if (this->acceptor_.acceptor ().get_local_addr (address) != 0)
    {
        ACE_ERROR ((LM_ERROR,
        ACE_TEXT ("(%P|%t) ERROR: SimpleTcpTransport::configure_i ")
                    ACE_TEXT ("- %p"),
                    ACE_TEXT ("cannot get local addr\n")));
    }
  unsigned short port = address.get_port_number ();

  // update this acceptor's copy.
  this->tcp_config_->local_address_.set_port_number (port);

  // update the caller's copy.
  // This is redundant because the local and caller's copy point
  // to the same place but just in case that changes. ;)
  if (tcp_config->local_address_.get_ip_address () == INADDR_ANY)
    {
      tcp_config->local_address_ = this->tcp_config_->local_address_;
    }
  tcp_config->local_address_.set_port_number (port);

  // Ahhh...  The sweet smell of success!
  return 0;
}

void
TAO::DCPS::SimpleTcpTransport::pre_shutdown_i()
{
  DBG_ENTRY("SimpleTcpTransport","pre_shutdown_i");
  
  GuardType guard(this->links_lock_);

  AddrLinkMap::ENTRY* entry;

  for (AddrLinkMap::ITERATOR itr(this->links_);
        itr.next(entry);
        itr.advance())
    {
      entry->int_id_->pre_stop_i();
    }
}


void
TAO::DCPS::SimpleTcpTransport::shutdown_i()
{
  DBG_ENTRY("SimpleTcpTransport","shutdown_i");
  
  // Don't accept any more connections.
  this->acceptor_.close();
  this->acceptor_.transport_shutdown ();

  this->reconnect_task_->shutdown ();

  {
    GuardType guard(this->connections_lock_);

    this->connections_.unbind_all();

    // TBD SOON - Need to set some flag to tell those threads waiting on
    //            the connections_updated_ condition that there is no hope
    //            of ever getting the connection that they seek - they should
    //            give up rather than continue to wait.

    // We need to signal all of the threads that may be stuck wait()'ing
    // on the connections_updated_ condition.
    this->connections_updated_.broadcast();
  }

  // Disconnect all of our DataLinks, and clear our links_ collection.
  {
    GuardType guard(this->links_lock_);

    AddrLinkMap::ENTRY* entry;

    for (AddrLinkMap::ITERATOR itr(this->links_);
         itr.next(entry);
         itr.advance())
      {
        entry->int_id_->transport_shutdown();
      }

    this->links_.unbind_all();
  }

  // Drop our reference to the SimpleTcpConfiguration object.
  this->tcp_config_ = 0;

  // Drop our reference to the TransportReactorTask
  this->reactor_task_ = 0;

  // Drop our reference to the TransportReconnectTask.
  this->reconnect_task_ = 0;

  // Tell our acceptor about this event so that it can drop its reference
  // it holds to this SimpleTcpTransport object (via smart-pointer).
  this->acceptor_.transport_shutdown();
}


int
TAO::DCPS::SimpleTcpTransport::connection_info_i
                                   (TransportInterfaceInfo& local_info) const
{
  DBG_ENTRY("SimpleTcpTransport","connection_info_i");

  NetworkAddress network_order_address(this->tcp_config_->local_address_);

  // Allow DCPSInfo to check compatibility of transport implemenations.
  local_info.transport_id = 1; // TBD Change magic number into a enum or constant value.
  local_info.data = TAO::DCPS::TransportInterfaceBLOB
                                    (sizeof(NetworkAddress),
                                     sizeof(NetworkAddress),
                                     (CORBA::Octet*)(&network_order_address));

  return 0;
}


void
TAO::DCPS::SimpleTcpTransport::release_datalink_i(DataLink* link)
{
  DBG_ENTRY("SimpleTcpTransport","release_datalink_i");

  SimpleTcpDataLink* tcp_link = ACE_static_cast(SimpleTcpDataLink*,link);

  if (tcp_link == 0)
    {
      // Really an assertion failure
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) INTERNAL ERROR - Failed to downcast DataLink to "
                 "SimpleTcpDataLink.\n"));
      return;
    }

  // Get the remote address from the SimpleTcpDataLink to be used as a key.
  ACE_INET_Addr remote_address = tcp_link->remote_address();

  SimpleTcpDataLink_rch released_link;

  GuardType guard(this->links_lock_);

  // Attempt to remove the SimpleTcpDataLink from our links_ map.
  if (this->links_.unbind(remote_address, released_link) != 0)
    {
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) ERROR: Unable to locate DataLink in order to "
                 "release and it.\n"));
    }
}


TAO::DCPS::SimpleTcpConfiguration*
TAO::DCPS::SimpleTcpTransport::get_configuration()
{
  return this->tcp_config_.in();
}


/// This method is called by a SimpleTcpConnection object that has been
/// created and opened by our acceptor_ as a result of passively
/// accepting a connection on our local address.  The connection object
/// is "giving itself away" for us to manage.  Ultimately, the connection
/// object needs to be paired with a DataLink object that is (or will be)
/// expecting this passive connection to be established.
void
TAO::DCPS::SimpleTcpTransport::passive_connection
                                        (const ACE_INET_Addr& remote_address,
                                         SimpleTcpConnection* connection)
{
  DBG_ENTRY("SimpleTcpTransport","passive_connection");
  // Take ownership of the passed-in connection pointer.
  SimpleTcpConnection_rch connection_obj = connection;

  {
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

  // Enqueue the connection to the reconnect task that verifies if the connection
  // is re-established.
  this->reconnect_task_->add (SimpleTcpReconnectTask::NEW_CONNECTION_CHECK, connection);
}


/// Actively establish a connection to the remote address.
int
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
                                       this->tcp_config_) != 0)
    {
      return -1;
    }

  return this->connect_datalink(link, connection.in());
}


int
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
int
TAO::DCPS::SimpleTcpTransport::connect_datalink
                                        (SimpleTcpDataLink*   link,
                                         SimpleTcpConnection* connection)
{
  DBG_ENTRY("SimpleTcpTransport","connect_datalink");

  ACE_Time_Value max_output_pause_period(this->tcp_config_->max_output_pause_period_/1000,
                                         this->tcp_config_->max_output_pause_period_%1000*1000); 
  TransportSendStrategy_rch send_strategy = 
             new SimpleTcpSendStrategy(link,
                                       this->tcp_config_.in(),
                                       connection,
                                       new SimpleTcpSynchResource(connection,
                                                                  max_output_pause_period));

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


/// This function is called by the SimpleTcpReconnectTask thread to check if the passively
/// accepted connection is the re-established connection. If it is, then the "old" connection
/// object in the datalink is replaced by the "new" connection object.
int
TAO::DCPS::SimpleTcpTransport::fresh_link (const ACE_INET_Addr&    remote_address,
                                           SimpleTcpConnection_rch connection)
{
  DBG_ENTRY("SimpleTcpTransport","fresh_link");

  SimpleTcpDataLink_rch link;
  GuardType guard(this->links_lock_);

  if (this->links_.find(remote_address,link) == 0)
    {
      SimpleTcpConnection_rch old_con = link->get_connection ();
      if (old_con.in () != connection.in ())
        // Replace the "old" connection object with the "new" connection object.
        return link->reconnect (connection.in ());
    }

  return 0;
}


