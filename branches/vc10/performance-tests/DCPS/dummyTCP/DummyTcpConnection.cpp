// -*- C++ -*-
//
// $Id$

#include "DummyTcp_pch.h"
#include "DummyTcpConnection.h"
#include "DummyTcpTransport.h"
#include "DummyTcpConfiguration.h"
#include "DummyTcpDataLink.h"
#include "DummyTcpReceiveStrategy.h"
#include "DummyTcpSendStrategy.h"
#include "DummyTcpReconnectTask.h"
#include "ace/os_include/netinet/os_tcp.h"

#if !defined (__ACE_INLINE__)
#include "DummyTcpConnection.inl"
#endif /* __ACE_INLINE__ */

#include "dds/DCPS/transport/framework/TransportReceiveStrategy.h"

// The connection lost can be detected by both send and receive strategy. When
// that happens, both of them add a request to the reconnect task. The reconnect
// will be attempted when the first request is dequeued and the second request
// just look the state to determine if the connection is good. To distinguish
// if the request is queued because the lost connection is detected by different
// threads or is because the re-established connection lost again, we need the
// reconnect_delay to help to identify these two cases so we can reset the reconnect
// state to trigger reconnecting after a re-established connection is lost.

// The reconnect delay is the period from the last time the reconnect attempt
// completes to when the reconnect request is dequeued.
const ACE_Time_Value reconnect_delay (2);

OpenDDS::DCPS::DummyTcpConnection::DummyTcpConnection()
: connected_ (false),
  is_connector_ (true),
  passive_reconnect_timer_id_ (-1),
  reconnect_task_ (this),
  reconnect_state_ (INIT_STATE),
  last_reconnect_attempted_ (ACE_Time_Value::zero),
  shutdown_ (false)
{
  DBG_ENTRY_LVL("DummyTcpConnection","DummyTcpConnection",5);

  // Open the reconnect task
  if (this->reconnect_task_.open ())
    {
      ACE_ERROR ((LM_ERROR,
                  ACE_TEXT ("(%P|%t) ERROR: Reconnect task failed to open : %p\n"),
                  ACE_TEXT ("open")));
    }
}

OpenDDS::DCPS::DummyTcpConnection::~DummyTcpConnection()
{
  DBG_ENTRY_LVL("DummyTcpConnection","~DummyTcpConnection",5);

  // Remove the reference of the old connection object
  // or the reference of new connection object.
  this->old_con_ = 0;
  this->new_con_ = 0;

  // The Reconnect task belongs to the Connection object.
  // Cleanup before leaving the house.
  this->reconnect_task_.close (1);
  //this->reconnect_task_.wait ();

  if (!this->link_.is_nil()) {
    this->link_->notify_connection_deleted ();
  }
}


// This can not be inlined due to circular dependencies disallowing
// visibility into the receive strategy to call add_ref().  Oh well.
void
OpenDDS::DCPS::DummyTcpConnection::set_receive_strategy
                                 (TransportReceiveStrategy* receive_strategy)
{
  DBG_ENTRY_LVL("DummyTcpConnection","set_receive_strategy",5);

  // Make a "copy" for ourselves
  receive_strategy->_add_ref();
  this->receive_strategy_ = receive_strategy;
}


void
OpenDDS::DCPS::DummyTcpConnection::set_send_strategy
                                 (DummyTcpSendStrategy* send_strategy)
{
  DBG_ENTRY_LVL("DummyTcpConnection","set_send_strategy",5);

  // Make a "copy" for ourselves
  send_strategy->_add_ref();
  this->send_strategy_ = send_strategy;
}


int
OpenDDS::DCPS::DummyTcpConnection::open(void* arg)
{
  DBG_ENTRY_LVL("DummyTcpConnection","open",5);

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
  // DummyTcpConnection object, and is also the caller of this open()
  // method.  We need to cast the arg to the DummyTcpAcceptor* type.
  DummyTcpAcceptor* acceptor = static_cast<DummyTcpAcceptor*>(arg);

  if (acceptor == 0)
    {
      // The cast failed.
      ACE_ERROR_RETURN((LM_ERROR,
                        "(%P|%t) ERROR: Failed to cast void* arg to "
                        "DummyTcpAcceptor* type.\n"),
                       -1);
    }

  // Now we need to ask the DummyTcpAcceptor object to provide us with
  // a pointer to the DummyTcpTransport object that "owns" the acceptor.
  DummyTcpTransport_rch transport = acceptor->transport();

  if (transport.is_nil())
    {
      // The acceptor gave us a nil transport (smart) pointer.
      ACE_ERROR_RETURN((LM_ERROR,
                        "(%P|%t) ERROR: Acceptor's transport is nil.\n"),
                       -1);
    }

  DummyTcpConfiguration* tcp_config = acceptor->get_configuration();

  // Keep a "copy" of the reference to DummyTcpConfiguration object
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
         ACE_TEXT("(%P|%t) ERROR: SimpleTcpConnection::open() - ")
         ACE_TEXT("unable to receive the length of address string ")
         ACE_TEXT("from the remote (active) side of the connection. ")
         ACE_TEXT("%p\n"),
         ACE_TEXT("recv_n")),
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


  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
    "DummyTcpConnection::open %X %C:%d->%C:%d reconnect_state = %d\n", this,
    this->remote_address_.get_host_addr (), this->remote_address_.get_port_number (),
    this->local_address_.get_host_addr (), this->local_address_.get_port_number (),
    this->reconnect_state_));

  // Now it is time to announce (and give) ourselves to the
  // DummyTcpTransport object.
  transport->passive_connection(this->remote_address_,this);

  this->connected_ = true;

  return 0;
}


int
OpenDDS::DCPS::DummyTcpConnection::handle_input(ACE_HANDLE)
{
  DBG_ENTRY_LVL("DummyTcpConnection","handle_input",5);

  TransportReceiveStrategy_rch rs = this->receive_strategy_;

  if (rs.is_nil())
    {
      return 0;
    }

  return rs->handle_input();
}


int
OpenDDS::DCPS::DummyTcpConnection::close(u_long)
{
  DBG_ENTRY_LVL("DummyTcpConnection","close",5);

  // TBD SOON - Find out exactly when close() is called.
  //            I have no clue when and who might call this.

  this->peer().close();
  this->connected_ = false;
  return 0;
}


int
OpenDDS::DCPS::DummyTcpConnection::handle_close(ACE_HANDLE, ACE_Reactor_Mask)
{
  DBG_ENTRY_LVL("DummyTcpConnection","handle_close",5);

  // TBD SOON - Find out exactly when handle_close() is called.
  //            My guess is that it happens if the reactor is closed
  //            while we are still registered with the reactor.  Right?

  this->peer().close();
  this->connected_ = false;
  return 0;
}

void
OpenDDS::DCPS::DummyTcpConnection::set_sock_options (DummyTcpConfiguration* tcp_config)
{
#if defined (ACE_DEFAULT_MAX_SOCKET_BUFSIZ)
  int snd_size = ACE_DEFAULT_MAX_SOCKET_BUFSIZ;
  int rcv_size = ACE_DEFAULT_MAX_SOCKET_BUFSIZ;
  //ACE_SOCK_Stream sock = ACE_static_cast(ACE_SOCK_Stream, this->peer() );
#  if !defined (ACE_LACKS_SOCKET_BUFSIZ)

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
      "(%P|%t) DummyTcpConnection failed to set the send buffer size to %d errno %m\n",
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
      "(%P|%t) DummyTcpConnection failed to set the receive buffer size to %d errno %m \n",
      rcv_size));
    return;
  }
#  else
  ACE_UNUSED_ARG (tcp_config);
  ACE_UNUSED_ARG (snd_size);
  ACE_UNUSED_ARG (rcv_size);
#  endif /* !ACE_LACKS_SOCKET_BUFSIZ */

#else
  ACE_UNUSED_ARG (tcp_config);
#endif /* !ACE_DEFAULT_MAX_SOCKET_BUFSIZ */
}


int
OpenDDS::DCPS::DummyTcpConnection::active_establishment
                                    (const ACE_INET_Addr& remote_address,
                                     const ACE_INET_Addr& local_address,
                                     DummyTcpConfiguration_rch tcp_config)
{
  DBG_ENTRY_LVL("DummyTcpConnection","active_establishment",5);

  // Cache these values for reconnecting.
  this->remote_address_ = remote_address;
  this->local_address_ = local_address;
  this->tcp_config_ = tcp_config;

  // Safty check - This should not happen since is_connector_ defaults to
  // true and the role in a connection connector is not changed when reconnecting.
  if (this->is_connector_ == false)
    {
      ACE_ERROR_RETURN((LM_ERROR,
                        "(%P|%t) ERROR: Failed to connect because it's previouly an acceptor.\n"),
                        -1);
    }

  if (this->shutdown_)
    return -1;

  // Now use a connector object to establish the connection.
  ACE_SOCK_Connector connector;
  if (connector.connect(this->peer(), remote_address) != 0)
    {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT ("(%P|%t) ERROR: Failed to connect. %p\n"),
                        ACE_TEXT ("connect")),
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
  NetworkAddress network_order_address(this->tcp_config_->local_address_str_);

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
OpenDDS::DCPS::DummyTcpConnection::reconnect (bool on_new_association)
{
  DBG_ENTRY_LVL("DummyTcpConnection","reconnect",5);

  if (on_new_association)
    return this->active_reconnect_on_new_association ();
  // Try to reconnect if it's connector previously.
  else if (this->is_connector_ && this->active_reconnect_i () == -1)
    return -1;
  // Schedule a timer to see if a incoming connection is accepted when timeout.
  else if (! this->is_connector_ && this->passive_reconnect_i () == -1)
    return -1;

  return 0;
}


int
OpenDDS::DCPS::DummyTcpConnection::active_reconnect_on_new_association ()
{
  DBG_ENTRY_LVL("DummyTcpConnection","active_reconnect_on_new_association",5);
  GuardType guard (this->reconnect_lock_);

  if (this->connected_ == true)
    return 0;
  else if (this->active_establishment (this->remote_address_,
                                       this->local_address_,
                                       this->tcp_config_) == 0)
  {
    this->reconnect_state_ = INIT_STATE;
    this->send_strategy_->resume_send ();
    return 0;
  }
  return -1;
}


// This method is called on acceptor side when the lost connection is detected.
// A timer is scheduled to check if a new connection is created within the
// passive_reconnect_duration_ period.
int
OpenDDS::DCPS::DummyTcpConnection::passive_reconnect_i ()
{
  DBG_ENTRY_LVL("DummyTcpConnection","passive_reconnect_i",5);
  GuardType guard (this->reconnect_lock_);

  // The passive_reconnect_timer_id_ is used as flag to allow the timer scheduled just once.
  if (this->reconnect_state_ == INIT_STATE)
    {
     // Mark the connection lost since the recv/send just failed.
     this->connected_ = false;

     if (this->tcp_config_->passive_reconnect_duration_ == 0)
        return -1;

      ACE_Time_Value timeout (this->tcp_config_->passive_reconnect_duration_/1000,
                              this->tcp_config_->passive_reconnect_duration_%1000 * 1000);
      DummyTcpReceiveStrategy* rs
        = dynamic_cast <DummyTcpReceiveStrategy*> (this->receive_strategy_.in ());

      this->reconnect_state_ = PASSIVE_WAITING_STATE;
      this->link_->notify (DataLink::DISCONNECTED);

      // Give a copy to reactor.
      this->_add_ref ();
      this->passive_reconnect_timer_id_ = rs->get_reactor()->schedule_timer(this, 0, timeout);

      if (this->passive_reconnect_timer_id_ == -1)
        {
          this->_remove_ref ();
          ACE_ERROR_RETURN((LM_ERROR,
            ACE_TEXT("(%P|%t) ERROR: SimpleTcpConnection::passive_reconnect_i")
            ACE_TEXT(", %p.\n"), ACE_TEXT("schedule_timer")),
                            -1);
        }
    }

  return 0;
}


// This is the active reconnect implementation. The backoff algorithm is used as the
// reconnect strategy. e.g.
// With conn_retry_initial_interval = 500, conn_retry_backoff_multiplier = 2.0 and
// conn_retry_attempts = 6 the reconnect attempts will be:
// - first at 0 seconds(upon detection of the disconnect)
// - second at 0.5 seconds
// - third at 1.0 (2*0.5) seconds
// - fourth at 2.0 (2*1.0) seconds
// - fifth at 4.0 (2*2.0) seconds
// - sixth at  8.0 (2*4.0) seconds
int
OpenDDS::DCPS::DummyTcpConnection::active_reconnect_i ()
{
  DBG_ENTRY_LVL("DummyTcpConnection","active_reconnect_i",5);

  GuardType guard (this->reconnect_lock_);
  int ret = -1;

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
    "active_reconnect_i(%C:%d->%C:%d) reconnect_state = %d\n",
    this->remote_address_.get_host_addr (), this->remote_address_.get_port_number (),
    this->local_address_.get_host_addr (), this->local_address_.get_port_number (),
    this->reconnect_state_));

  // We need reset the state to INIT_STATE if we are previously reconnected.
  // This would allow re-establishing connection after the re-established
  // connection lost again.
  if (ACE_OS::gettimeofday () - this->last_reconnect_attempted_ > reconnect_delay
    && this->reconnect_state_ == RECONNECTED_STATE)
    {
      VDBG((LM_DEBUG, "(%P|%t) DBG:   "
        "We are in RECONNECTED_STATE and now flip reconnect state to INIT_STATE.\n"));
      this->reconnect_state_ = INIT_STATE;
    }

  if (this->reconnect_state_ == INIT_STATE)
    {
      // Suspend send once.
      this->send_strategy_->suspend_send ();

      this->peer ().close ();
      this->connected_ = false;
      if (this->tcp_config_->conn_retry_attempts_ > 0)
      {
        this->link_->notify (DataLink::DISCONNECTED);
      }
      // else the conn_retry_attempts is 0 then we do not need this extra
      // notify_disconnected() since the user application should get the
      // notify_lost() without delay.

      double retry_delay_msec = this->tcp_config_->conn_retry_initial_delay_;
      for (int i = 0; i < this->tcp_config_->conn_retry_attempts_; ++i)
      {
        ret = this->active_establishment (this->remote_address_,
          this->local_address_,
          this->tcp_config_);

        if (this->shutdown_)
          break;
        if (ret == -1)
        {
          ACE_Time_Value delay_tv (((int)retry_delay_msec)/1000,
            ((int)retry_delay_msec)%1000*1000);
          ACE_OS::sleep (delay_tv);
          retry_delay_msec *= this->tcp_config_->conn_retry_backoff_multiplier_;
        }
        else
        {
          break;
        }
      }

      if (ret == -1)
        {
          if (this->tcp_config_->conn_retry_attempts_ > 0)
            {
              ACE_DEBUG ((LM_DEBUG, "(%P|%t) we tried and failed to re-establish connection to %C:%d.\n",
                          this->remote_address_.get_host_addr (),
                          this->remote_address_.get_port_number ()));
            }
          else
            {
              ACE_DEBUG ((LM_DEBUG, "(%P|%t) we did not try to re-establish connection to %C:%d.\n",
                          this->remote_address_.get_host_addr (),
                          this->remote_address_.get_port_number ()));
            }

          this->reconnect_state_ = LOST_STATE;
          this->link_->notify (DataLink::LOST);
          this->send_strategy_->terminate_send ();
        }
      else
        {
          ACE_DEBUG ((LM_DEBUG, "(%P|%t)re-established connection to %C:%d.\n",
            this->remote_address_.get_host_addr (),
            this->remote_address_.get_port_number ()));
          this->reconnect_state_ = RECONNECTED_STATE;
          this->link_->notify (DataLink::RECONNECTED);
          this->send_strategy_->resume_send ();
        }

      this->last_reconnect_attempted_ = ACE_OS::gettimeofday ();
    }

    return this->reconnect_state_ == LOST_STATE ? -1 : 0;
}


/// A timer is scheduled on acceptor side to check if a new connection
/// is accepted after the connection is lost.
int
OpenDDS::DCPS::DummyTcpConnection::handle_timeout (const ACE_Time_Value &,
                                                const void *)
{
  DBG_ENTRY_LVL("DummyTcpConnection","handle_timeout",5);

  this->reconnect_state_ = PASSIVE_TIMEOUT_CALLED_STATE;
  GuardType guard (this->reconnect_lock_);

  switch (this->reconnect_state_)
  {
  case PASSIVE_TIMEOUT_CALLED_STATE:
    {
      // We stay in PASSIVE_TIMEOUT_CALLED_STATE indicates there is no new connection.
      // Now we need declare the connection is lost.
      this->link_->notify (DataLink::LOST);
      this->send_strategy_->terminate_send ();
      this->reconnect_state_ = LOST_STATE;
    }
    break;
  case RECONNECTED_STATE:
    // reconnected successfully.
    break;
  default :
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: DummyTcpConnection::handle_timeout, ")
      ACE_TEXT(" unknown state or it should not be in state=%d \n"), this->reconnect_state_));
    break;
  }

  // Take back the "copy" we gave to reactor when we schedule the timer.
  this->_remove_ref ();

  return 0;
}


/// This object would be "old" connection object and the provided is the new
/// connection object.  The "old" connection object will copy its states to
/// to the "new" connection object. This is called by the DummyTcpDataLink
/// when a new connection is accepted (with a new DummyTcpConnection object).
/// We need make the state in "new" connection object consistent with the "old"
/// connection object.
void
OpenDDS::DCPS::DummyTcpConnection::transfer (DummyTcpConnection* connection)
{
  DBG_ENTRY_LVL("DummyTcpConnection","transfer",5);

  GuardType guard (this->reconnect_lock_);

  bool notify_reconnect = false;

  switch (this->reconnect_state_)
  {
  case INIT_STATE:
      // We have not detected the lost connection and the peer is faster than us and
      // re-established the connection. so do not notify reconnected.
    break;
  case LOST_STATE:
      // The reconnect timed out.
  case PASSIVE_TIMEOUT_CALLED_STATE:
      // TODO: If the handle_timeout is called before the old connection
      // transfer its state to new connection then should we disconnect
      // the new connection or keep it alive ?
      // I think we should keep the connection, the user will get a
      // lost connection notification and then a reconnected notification.
    notify_reconnect = true;
    break;
  case PASSIVE_WAITING_STATE:
    {
      DummyTcpReceiveStrategy* rs
        = dynamic_cast <DummyTcpReceiveStrategy*> (this->receive_strategy_.in ());

      // Cancel the timer since we got new connection.
      if (rs->get_reactor()->cancel_timer(this) == -1)
      {
        ACE_ERROR((LM_ERROR,
          ACE_TEXT("(%P|%t) ERROR: SimpleTcpConnection::transfer, ")
          ACE_TEXT(" %p. \n"), ACE_TEXT("cancel_timer")));
      }
      this->_remove_ref ();
      notify_reconnect = true;
    }
    break;
  default :
    ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: DummyTcpConnection::transfer, ")
        ACE_TEXT(" unknown state or it should not be in state=%d \n"), this->reconnect_state_));
    break;
  }

  // Verify if this acceptor side.
  if (this->is_connector_ || connection->is_connector_)
    {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: DummyTcpConnection::transfer, ")
        ACE_TEXT(" should NOT be called by the connector side \n")));
    }

  this->reconnect_task_.close (1);
  connection->receive_strategy_ = this->receive_strategy_;
  connection->send_strategy_ = this->send_strategy_;
  connection->remote_address_ = this->remote_address_;
  connection->local_address_ = this->local_address_;
  connection->tcp_config_ = this->tcp_config_;
  connection->link_ = this->link_;

  //Make the "old" and "new" connection object keep a copy each other.
  //Note only does the "old" connection object call this transfer () function
  //since we need use the lock to synch this function and handle_timeout.
  connection->_add_ref ();
  this->new_con_ = connection;

  this->_add_ref ();
  connection->old_con_ = this;

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
        "transfer(%C:%d->%C:%d) passive reconnected. new con %X   "
        " old con %X \n",
        this->remote_address_.get_host_addr (), this->remote_address_.get_port_number (),
        this->local_address_.get_host_addr (), this->local_address_.get_port_number (),
        connection, this));

  if (notify_reconnect)
    {
      this->reconnect_state_ = RECONNECTED_STATE;
      this->link_->notify (DataLink::RECONNECTED);
    }
}


/// This function is called when the backpresure occurs and timed out after
/// "max_output_pause_period". The lost connection notification should be sent
/// and the connection needs be closed since we declared it as a "lost"
/// connection.
void
OpenDDS::DCPS::DummyTcpConnection::notify_lost_on_backpressure_timeout ()
{
  DBG_ENTRY_LVL("DummyTcpConnection","notify_lost_on_backpressure_timeout",5);
  bool notify_lost = false;
  {
    GuardType guard (this->reconnect_lock_);
    if (this->reconnect_state_ == INIT_STATE)
      {
        this->reconnect_state_ = LOST_STATE;
        notify_lost = true;
        this->peer ().close ();
        this->connected_ = false;
      }
  }

  if (notify_lost)
    {
      this->link_->notify (DataLink::LOST);
      this->send_strategy_->terminate_send ();
    }
}


/// This is called by both DummyTcpSendStrategy and DummyTcpReceiveStrategy
/// when lost connection is detected. This method handles the connection
/// to the reactor task to do the reconnecting.
void
OpenDDS::DCPS::DummyTcpConnection::relink (bool do_suspend)
{
  DBG_ENTRY_LVL("DummyTcpConnection","relink",5);

  if (do_suspend && ! this->send_strategy_.is_nil ())
    this->send_strategy_->suspend_send ();

  ReconnectOpType op = DO_RECONNECT;
  this->reconnect_task_.add (op);
}

bool
OpenDDS::DCPS::DummyTcpConnection::tear_link ()
{
  DBG_ENTRY_LVL("DummyTcpConnection","tear_link",5);

  return this->link_->release_resources ();
}

void
OpenDDS::DCPS::DummyTcpConnection::shutdown ()
{
  DBG_ENTRY_LVL("DummyTcpConnection","shutdown",5);
  this->shutdown_ = true;

  this->reconnect_task_.close (1);
}
