// -*- C++ -*-
//
// $Id$

#include  "DCPS/DdsDcps_pch.h"
#include  "SimpleTcpConnection.h"
#include  "SimpleTcpTransport.h"
#include  "SimpleTcpConfiguration.h"
#include  "SimpleTcpDataLink.h"
#include  "SimpleTcpReceiveStrategy.h"
#include  "ace/os_include/netinet/os_tcp.h"

#if !defined (__ACE_INLINE__)
#include "SimpleTcpConnection.inl"
#endif /* __ACE_INLINE__ */

#include  "dds/DCPS/transport/framework/TransportReceiveStrategy.h"

TAO::DCPS::SimpleTcpConnection::~SimpleTcpConnection()
{
  DBG_ENTRY("SimpleTcpConnection","~SimpleTcpConnection");
}


// This can not be inlined due to circular dependencies disallowing
// visibility into the receive strategy to call add_ref().  Oh well.
void
TAO::DCPS::SimpleTcpConnection::set_receive_strategy
                                 (TransportReceiveStrategy* receive_strategy)
{
  DBG_ENTRY("SimpleTcpConnection","set_receive_strategy");

  // Make a "copy" for ourselves
  receive_strategy->_add_ref();
  this->receive_strategy_ = receive_strategy;
}

int
TAO::DCPS::SimpleTcpConnection::open(void* arg)
{
  DBG_ENTRY("SimpleTcpConnection","open");

  // A safety check - This should not happen since the is_connector_ 
  // defaults to true and open() is called after the ACE_Aceptor 
  // creates this new svc handler.
  if (this->is_connector_ == false)
  	{
  	  return -1;	
  	}
  
  // This connection object represents the acceptor side.
  this->is_connector_ = false;
  
  // The passed-in arg is really the acceptor object that created this
  // SimpleTcpConnection object, and is also the caller of this open()
  // method.  We need to cast the arg to the SimpleTcpAcceptor* type.
  SimpleTcpAcceptor* acceptor = ACE_static_cast(SimpleTcpAcceptor*,arg);

  if (acceptor == 0)
    {
      // The cast failed.
      ACE_ERROR_RETURN((LM_ERROR,
                        "(%P|%t) ERROR: Failed to cast void* arg to "
                        "SimpleTcpAcceptor* type.\n"),
                       -1);
    }

  // Now we need to ask the SimpleTcpAcceptor object to provide us with
  // a pointer to the SimpleTcpTransport object that "owns" the acceptor.
  SimpleTcpTransport_rch transport = acceptor->transport();

  if (transport.is_nil())
    {
      // The acceptor gave us a nil transport (smart) pointer.
      ACE_ERROR_RETURN((LM_ERROR,
                        "(%P|%t) ERROR: Acceptor's transport is nil.\n"),
                       -1);
    }

  SimpleTcpConfiguration* tcp_config = acceptor->get_configuration();
  
  // Keep a "copy" of the reference to SimpleTcpConfiguration object 
  // for ourselves.
  tcp_config->_add_ref (); 
  this->tcp_config_ = tcp_config;

  set_sock_options(this->tcp_config_.in ());

  // We expect that the active side of the connection (the remote side
  // in this case) will supply its listening ACE_INET_Addr as the first
  // message it sends to the socket.  This is a one-way connection
  // establishment protocol message.
  NetworkAddress network_order_address;

  if (this->peer().recv_n((char*)(&network_order_address),
                          sizeof(network_order_address)) == -1)
    {
      ACE_ERROR_RETURN((LM_ERROR,
                        "(%P|%t) ERROR: Unable to receive the remote_address "
                        "from the remote (active) side of the connection."
                        " %p\n", "recv_n"),
                       -1);
    }

  network_order_address.to_addr(this->remote_address_);

//MJM: vvv CONNECTION ESTABLISHMENT CHANGES vvv

//MJM: Add code to send a response to the other side that the
//MJM: connection is ready to receive at this point.  It may be
//MJM: necessary to do this higher in the layers to make sure that we
//MJM: really are ready to receive.

//MJM: This is the only really tricky bit, since I have not really
//MJM: investigated enough to know where the connection is considered
//MJM: complete on this end.  I think that it will be when the datalink
//MJM: is placed in all the containers.

//MJM: This is also where this end needs to call back the
//MJM: TransportInterface method that will eventually signal() the
//MJM: wait()ing add_associations() call.  It may not be necessary on
//MJM: this (passive) to actually perform the wait() and signal()
//MJM: operations.  There is enough information in the
//MJM: add_associations() call to differentiate the cases.

//MJM: ^^^ CONNECTION ESTABLISHMENT CHANGES ^^^

  // Now it is time to announce (and give) ourselves to the
  // SimpleTcpTransport object.
  transport->passive_connection(this->remote_address_,this);
  
  this->connected_ = true;

  return 0;
}


int
TAO::DCPS::SimpleTcpConnection::handle_input(ACE_HANDLE)
{
  DBG_ENTRY("SimpleTcpConnection","handle_input");

  TransportReceiveStrategy_rch rs = this->receive_strategy_;

  if (rs.is_nil())
    {
      return 0;
    }

  return rs->handle_input();
}


int
TAO::DCPS::SimpleTcpConnection::close(u_long)
{
  DBG_ENTRY("SimpleTcpConnection","close");

  // TBD SOON - Find out exactly when close() is called.
  //            I have no clue when and who might call this.

  this->peer().close();  
  this->connected_ = false;
  return 0;
}


int
TAO::DCPS::SimpleTcpConnection::handle_close(ACE_HANDLE, ACE_Reactor_Mask)
{
  DBG_ENTRY("SimpleTcpConnection","handle_close");

  // TBD SOON - Find out exactly when handle_close() is called.
  //            My guess is that it happens if the reactor is closed
  //            while we are still registered with the reactor.  Right?

  this->peer().close();
  this->connected_ = false;
  return 0;
}

void
TAO::DCPS::SimpleTcpConnection::set_sock_options (SimpleTcpConfiguration* tcp_config)
{
#if defined (ACE_DEFAULT_MAX_SOCKET_BUFSIZ)
  int snd_size = ACE_DEFAULT_MAX_SOCKET_BUFSIZ;
  int rcv_size = ACE_DEFAULT_MAX_SOCKET_BUFSIZ;
  //ACE_SOCK_Stream sock = ACE_static_cast(ACE_SOCK_Stream, this->peer() );
#if !defined (ACE_LACKS_SOCKET_BUFSIZ)

  // A little screwy double negative logic: disabling nagle involves
  // enabling TCP_NODELAY
  int opt = (tcp_config->enable_nagle_algorithm_ == false);
  if (this->peer().set_option (IPPROTO_TCP, TCP_NODELAY, &opt, sizeof (opt)) == -1) {
    ACE_ERROR((LM_ERROR, "Failed to set TCP_NODELAY\n"));
  }

 if (this->peer().set_option (SOL_SOCKET,
                          SO_SNDBUF,
                          (void *) &snd_size,
                          sizeof (snd_size)) == -1
      && errno != ENOTSUP)
  {
    ACE_ERROR((LM_ERROR,
      "(%P|%t) SimpleTcpConnection failed to set the send buffer size to %d errno %m\n",
      snd_size));
    return;
  }

  if (this->peer().set_option (SOL_SOCKET,
                          SO_RCVBUF,
                          (void *) &rcv_size,
                          sizeof (int)) == -1
      && errno != ENOTSUP)
  {
    ACE_ERROR((LM_ERROR,
      "(%P|%t) SimpleTcpConnection failed to set the receive buffer size to %d errno %m \n",
      rcv_size));
    return;
  }
#else
   ACE_UNUSED_ARG (snd_size);
   ACE_UNUSED_ARG (rcv_size);
#endif /* !ACE_LACKS_SOCKET_BUFSIZ */

#else 
   ACE_UNUSED_ARG (snd_size);
   ACE_UNUSED_ARG (rcv_size);
#endif /* !ACE_DEFAULT_MAX_SOCKET_BUFSIZ */
}


int
TAO::DCPS::SimpleTcpConnection::active_establishment
                                    (const ACE_INET_Addr& remote_address,
                                     const ACE_INET_Addr& local_address,
                                     SimpleTcpConfiguration_rch tcp_config)
{
  DBG_ENTRY("SimpleTcpConnection","active_establishment");

  // Cache these values for reconnecting.
  this->remote_address_ = remote_address;
  this->local_address_ = local_address;
  this->tcp_config_ = tcp_config;
  
  // Safty check - This should not happen since is_connector_ defaults to
  // true and the role in a connection connector is not changed when reconnecting.
  if (this->is_connector_ == false)
    {
      ACE_ERROR_RETURN((LM_ERROR,
                        "(%P|%t) ERROR: Failed to connect. %p\n", 
                        "connect"), -1);
    }

  // Now use a connector object to establish the connection.
  ACE_SOCK_Connector connector;
  if (connector.connect(this->peer(), remote_address) != 0)
    {
      ACE_ERROR_RETURN((LM_ERROR,
                        "(%P|%t) ERROR: Failed to connect.\n"),
                       -1);
    }
  else
    {
      this->connected_ = true;
    }

  set_sock_options(tcp_config.in ());

  // In order to complete the connection establishment from the active
  // side, we need to tell the remote side about our local_address.
  // It will use that as an "identifier" of sorts.  To the other
  // (passive) side, our local_address that we send here will be known
  // as the remote_address.
  NetworkAddress network_order_address(local_address);

  if (this->peer().send_n((char*)(&network_order_address),
                          sizeof(network_order_address)) == -1)
    {
      // TBD later - Anything we are supposed to do to close the connection.
      ACE_ERROR_RETURN((LM_ERROR,
                        "(%P|%t) ERROR: Unable to send our local_address_ to "
                        "the passive side to complete the active connection "
                        "establishment.\n"),
                       -1);
    }

//MJM: vvv CONNECTION ESTABLISHMENT CHANGES vvv

//MJM: Add code to receive a response from the other side that the
//MJM: connection is ready to receive at this point.  Block until it is
//MJM: received.  Then call the method in the TransportInterface that
//MJM: the add_associations() call is wait()ing on.

//MJM: ^^^ CONNECTION ESTABLISHMENT CHANGES ^^^

  return 0;
}


/// This function is called to re-establish the connection. If this object 
/// is the connector side of the connection then it tries to reconnect to the
/// remote, if it's the acceptor side of the connection then it schedules a timer
/// to check if it passively accepted a connection from remote.
/// The on_new_association true indicates this is called when the connection is
/// previous lost and new association is added. The connector side needs to try to
/// actively reconnect to remote.
int
TAO::DCPS::SimpleTcpConnection::reconnect (bool on_new_association)
{
  DBG_ENTRY("SimpleTcpConnection","reconnect");
  // Try to reconnect if it's connector previously.
  if (this->is_connector_ && this->reconnect_i (on_new_association) == -1)
    return -1;
   
  // Schedule a timer to see if a incoming connection is accepted when timeout.
  if (! this->is_connector_)
    {
      // Mark the connection lost since the recv just failed.
      this->connected_ = false;

      // Give a copy to reactor.
      this->_add_ref ();

      if (this->tcp_config_->passive_reconnect_duration_ == 0)
        return -1;

      ACE_Time_Value timeout (this->tcp_config_->passive_reconnect_duration_/1000,
                              this->tcp_config_->passive_reconnect_duration_%1000 * 1000);
      SimpleTcpReceiveStrategy* rs 
        = dynamic_cast <SimpleTcpReceiveStrategy*> (this->receive_strategy_.in ());
      if (rs->get_reactor()->schedule_timer(this, 0, timeout) == -1)
        {
          this->_remove_ref ();
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) ERROR: SimpleTcpConnection::reconnect, ")
                            ACE_TEXT(" %p. \n"), "schedule_timer" ),
                            -1);
        }
    }

  return 0;
}


// This is the reconnect implementation. The backoff algorithm is used as the reconnect
// strategy. e.g. 
// With conn_retry_initial_interval = 500, conn_retry_backoff_multiplier = 2.0 and
// conn_retry_attempts = 6 the reconnect attempts will be:
// - first at 0 seconds(upon detection of the disconnect)
// - second at 0.5 seconds
// - third at 1.0 (2*0.5) seconds
// - fourth at 2.0 (2*1.0) seconds
// - fifth at 4.0 (2*2.0) seconds
// - sixth at  8.0 (2*4.0) seconds
int
TAO::DCPS::SimpleTcpConnection::reconnect_i (bool on_new_association)
{
  DBG_ENTRY("SimpleTcpConnection","reconnect_i");
      
  int ret = -1;
  bool notify_lost = false;
  bool connected = false;
  {
    GuardType guard (this->reconnect_lock_);

    // Try to reconnect if the connection just lost or the connection was lost 
    // previously and now new association is coming.
    if (! this->connection_lost_notified_ || on_new_association)
      {
        int conn_retry_attempts = this->tcp_config_->conn_retry_attempts_;
        if (on_new_association)
          {
            // If reconnect() is called upon the new association, then 
            // we need try reconnect at least once even though the
            // the "conn_retry_attempts" is zero, 
            this->connection_lost_notified_ = false;
            if (conn_retry_attempts == 0)
              conn_retry_attempts = 1;
          }
        this->peer ().close ();
        this->connected_ = false;

        double retry_delay_msec = this->tcp_config_->conn_retry_initial_delay_;
        for (int i = 0; i < conn_retry_attempts; ++i)
          {
            ret = this->active_establishment (this->remote_address_,
              this->local_address_,
              this->tcp_config_);
            if (ret == -1)
              {
                ACE_Time_Value delay_tv (((int)retry_delay_msec)/1000, 
                                         ((int)retry_delay_msec)%1000*1000);
                ACE_OS::sleep (delay_tv);
                retry_delay_msec *= this->tcp_config_->conn_retry_backoff_multiplier_;
              }
            else
              {
                ACE_DEBUG ((LM_DEBUG, "(%P|%t)re-established lost connection to %s:%d.\n",
                  this->remote_address_.get_host_addr (), 
                  this->remote_address_.get_port_number ()));
                break;
              }
          }

        if (ret == -1)
          {
            ACE_DEBUG ((LM_DEBUG, "(%P|%t) failed to re-establish lost connection to %s:%d.\n",
                        this->remote_address_.get_host_addr (), 
                        this->remote_address_.get_port_number ()));
            this->connection_lost_notified_ = true;
            notify_lost = true;
          }
      }

    connected = this->connected_;
  }  


  if (notify_lost)
    {
      this->link_->notify_lost ();
    }

  return connected ? 0 : -1;
}


/// A timer is scheduled on acceptor side to check if a new connection 
/// is accepted after the connection is lost. 
int
TAO::DCPS::SimpleTcpConnection::handle_timeout (const ACE_Time_Value &,
                                                const void *)
{
  DBG_ENTRY("SimpleTcpConnection","handle_timeout");

  bool connected 
    = this->conn_replacement_.is_nil () ? this->connected_ : this->conn_replacement_->connected_;

  if (! connected)
    {
      this->link_->notify_lost ();
    }

  // Take back the "copy" we gave to reactor when we schedule the timer.
  this->_remove_ref ();
  return 0;
}


/// Copy the states from an "old" SimpleTcpConnection object associated with 
/// a SimpleTcpDataLink to the new SimpleTcpConnection object.
/// This is called by the SimpleTcpDataLink when a new connection is accepted
/// (with a new SimpleTcpConnection object). We need replace the "old" SimpleTcpConnection
/// object with the new one.
void 
TAO::DCPS::SimpleTcpConnection::copy_states (SimpleTcpConnection* connection)
{
  DBG_ENTRY("SimpleTcpConnection","copy_states");
  this->connected_ = connection->connected_;
  this->is_connector_ = connection->is_connector_;
  this->receive_strategy_ = connection->receive_strategy_;
  this->remote_address_ = connection->remote_address_;
  this->local_address_ = connection->local_address_;
  this->tcp_config_ = connection->tcp_config_;
  this->link_ = connection->link_;

  // Give a copy to the "old" connection object since the timer is schedule with the 
  // "old" SimpleTcpConnection object.
  this->_add_ref ();
  connection->conn_replacement_ = this;
  this->connection_lost_notified_ = connection->connection_lost_notified_;
}


/// This function is called when the backpresure occurs and timed out after
/// "max_output_pause_period". The lost connection notification should be sent
/// and the connection needs be closed since we declared it as a "lost" 
/// connection. 
void 
TAO::DCPS::SimpleTcpConnection::notify_lost_on_backpressure_timeout ()
{
  DBG_ENTRY("SimpleTcpConnection","notify_lost_on_backpressure_timeout");
  bool notify_lost = false;  
  {
    GuardType guard (this->reconnect_lock_);
    if (this->connection_lost_notified_ == false)
      {
        this->connection_lost_notified_ = true;
        notify_lost = true;
        // Disconnect the connection
        if (this->connected_)
          {
            this->peer ().close ();
            this->connected_ = false;
          }
      }
  }

  if (notify_lost)
    this->link_->notify_lost ();
}

