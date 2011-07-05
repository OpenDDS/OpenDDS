/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "Tcp_pch.h"
#include "TcpConnection.h"
#include "TcpTransport.h"
#include "TcpInst.h"
#include "TcpDataLink.h"
#include "TcpReceiveStrategy.h"
#include "TcpSendStrategy.h"
#include "TcpReconnectTask.h"
#include "dds/DCPS/transport/framework/DirectPriorityMapper.h"
#include "ace/os_include/netinet/os_tcp.h"
#include "ace/OS_NS_arpa_inet.h"
#include <sstream>

#if !defined (__ACE_INLINE__)
#include "TcpConnection.inl"
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
const ACE_Time_Value reconnect_delay(2);

OpenDDS::DCPS::TcpConnection::TcpConnection()
  : connected_(false),
    is_connector_(true),
    passive_reconnect_timer_id_(-1),
    reconnect_task_(this),
    reconnect_state_(INIT_STATE),
    last_reconnect_attempted_(ACE_Time_Value::zero),
    transport_priority_(0),  // TRANSPORT_PRIORITY.value default value - 0.
    shutdown_(false)
{
  DBG_ENTRY_LVL("TcpConnection","TcpConnection",6);

  // Open the reconnect task
  if (this->reconnect_task_.open()) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: Reconnect task failed to open : %p\n"),
               ACE_TEXT("open")));
  }
}

OpenDDS::DCPS::TcpConnection::~TcpConnection()
{
  DBG_ENTRY_LVL("TcpConnection","~TcpConnection",6);

  // Remove the reference of the old connection object
  // or the reference of new connection object.
  this->old_con_ = 0;
  this->new_con_ = 0;

  // The Reconnect task belongs to the Connection object.
  // Cleanup before leaving the house.
  this->reconnect_task_.close(1);
  //this->reconnect_task_.wait ();

  if (!this->link_.is_nil()) {
    this->link_->notify_connection_deleted();
  }
}

void
OpenDDS::DCPS::TcpConnection::disconnect()
{
  DBG_ENTRY_LVL("TcpConnection","disconnect",6);
  this->connected_ = false;

  if (!this->receive_strategy_.is_nil()) {

    this->receive_strategy_->get_reactor()->remove_handler(this,
                                      ACE_Event_Handler::READ_MASK | ACE_Event_Handler::DONT_CALL);
  }

  this->peer().close();
}

// This can not be inlined due to circular dependencies disallowing
// visibility into the receive strategy to call add_ref().  Oh well.
void
OpenDDS::DCPS::TcpConnection::set_receive_strategy
(TcpReceiveStrategy* receive_strategy)
{
  DBG_ENTRY_LVL("TcpConnection","set_receive_strategy",6);

  // Make a "copy" for ourselves
  receive_strategy->_add_ref();
  this->receive_strategy_ = receive_strategy;
}

void
OpenDDS::DCPS::TcpConnection::set_send_strategy
(TcpSendStrategy* send_strategy)
{
  DBG_ENTRY_LVL("TcpConnection","set_send_strategy",6);

  // Make a "copy" for ourselves
  send_strategy->_add_ref();
  this->send_strategy_ = send_strategy;
}

int
OpenDDS::DCPS::TcpConnection::open(void* arg)
{
  DBG_ENTRY_LVL("TcpConnection","open",6);

  // A safety check - This should not happen since the is_connector_
  // defaults to true and open() is called after the ACE_Aceptor
  // creates this new svc handler.
  if (this->is_connector_ == false) {
    return -1;
  }

  // This connection object represents the acceptor side.
  this->is_connector_ = false;

  // The passed-in arg is really the acceptor object that created this
  // TcpConnection object, and is also the caller of this open()
  // method.  We need to cast the arg to the TcpAcceptor* type.
  TcpAcceptor* acceptor = static_cast<TcpAcceptor*>(arg);

  if (acceptor == 0) {
    // The cast failed.
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: TcpConnection::open() - ")
                      ACE_TEXT("failed to cast void* arg to ")
                      ACE_TEXT("TcpAcceptor* type.\n")),
                     -1);
  }

  // Now we need to ask the TcpAcceptor object to provide us with
  // a pointer to the TcpTransport object that "owns" the acceptor.
  TcpTransport_rch transport = acceptor->transport();

  if (transport.is_nil()) {
    // The acceptor gave us a nil transport (smart) pointer.
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: TcpConnection::open() - ")
                      ACE_TEXT("acceptor's transport is nil.\n")),
                     -1);
  }

  TcpInst* tcp_config = acceptor->get_configuration();

  // Keep a "copy" of the reference to TcpInst object
  // for ourselves.
  tcp_config->_add_ref();
  this->tcp_config_ = tcp_config;

  set_sock_options(this->tcp_config_.in());

  // We expect that the active side of the connection (the remote side
  // in this case) will supply its listening ACE_INET_Addr as the first
  // message it sends to the socket.  This is a one-way connection
  // establishment protocol message.

  ACE_UINT32 nlen = 0;

  if (this->peer().recv_n(&nlen,
                          sizeof(ACE_UINT32)) == -1) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: TcpConnection::open() - ")
                      ACE_TEXT("unable to receive the length of address string ")
                      ACE_TEXT("from the remote (active) side of the connection. ")
                      ACE_TEXT("%p\n"),
                      ACE_TEXT("recv_n")),
                     -1);
  }

  ACE_UINT32 len = ntohl(nlen);

  char * buf = new char [len];

  if (this->peer().recv_n(buf,
                          len) == -1) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: TcpConnection::open() - ")
                      ACE_TEXT("unable to receive the address string ")
                      ACE_TEXT("from the remote (active) side of the connection. ")
                      ACE_TEXT("%p\n"),
                      ACE_TEXT("recv_n")),
                     -1);
  }

  const std::string bufstr(buf);
  NetworkAddress network_order_address(bufstr);

  network_order_address.to_addr(this->remote_address_);

  delete[] buf;

  ACE_UINT32 priority = 0;

  if (this->peer().recv_n(&priority, sizeof(ACE_UINT32)) == -1) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: TcpConnection::open() - ")
                      ACE_TEXT("unable to receive the publication transport priority ")
                      ACE_TEXT("from the remote (active) side of the connection. ")
                      ACE_TEXT("%p\n"),
                      ACE_TEXT("recv_n")),
                     -1);
  }

  this->transport_priority_ = ntohl(priority);

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
        "TcpConnection::open %X %C:%d->%C:%d, priority==%d, reconnect_state = %d\n", this,
        this->remote_address_.get_host_addr(), this->remote_address_.get_port_number(),
        this->local_address_.get_host_addr(), this->local_address_.get_port_number(),
        this->transport_priority_,
        this->reconnect_state_));

  // Now it is time to announce (and give) ourselves to the
  // TcpTransport object.
  transport->passive_connection(this->remote_address_,this);

  this->connected_ = true;

  return 0;
}

int
OpenDDS::DCPS::TcpConnection::handle_input(ACE_HANDLE)
{
  DBG_ENTRY_LVL("TcpConnection","handle_input",6);

  if (this->receive_strategy_.is_nil()) {
    return 0;
  }

  return this->receive_strategy_->handle_input();
}

int
OpenDDS::DCPS::TcpConnection::close(u_long)
{
  DBG_ENTRY_LVL("TcpConnection","close",6);

  // TBD SOON - Find out exactly when close() is called.
  //            I have no clue when and who might call this.

  if (!this->send_strategy_.is_nil())
    this->send_strategy_->terminate_send();

  this->disconnect();
  return 0;
}

int
OpenDDS::DCPS::TcpConnection::handle_close(ACE_HANDLE, ACE_Reactor_Mask)
{
  DBG_ENTRY_LVL("TcpConnection","handle_close",6);

  // TBD SOON - Find out exactly when handle_close() is called.
  //            My guess is that it happens if the reactor is closed
  //            while we are still registered with the reactor.  Right?


  if (!this->send_strategy_.is_nil())
    this->send_strategy_->terminate_send();

  this->disconnect();

  if (!this->receive_strategy_.is_nil() && this->receive_strategy_->gracefully_disconnected())
  {
    this->link_->notify (DataLink::DISCONNECTED);
  }

  return 0;
}

void
OpenDDS::DCPS::TcpConnection::set_sock_options(TcpInst* tcp_config)
{
#if defined (ACE_DEFAULT_MAX_SOCKET_BUFSIZ)
  int snd_size = ACE_DEFAULT_MAX_SOCKET_BUFSIZ;
  int rcv_size = ACE_DEFAULT_MAX_SOCKET_BUFSIZ;
  //ACE_SOCK_Stream sock = ACE_static_cast(ACE_SOCK_Stream, this->peer() );
#  if !defined (ACE_LACKS_SOCKET_BUFSIZ)

  // A little screwy double negative logic: disabling nagle involves
  // enabling TCP_NODELAY
  int opt = (tcp_config->enable_nagle_algorithm_ == false);

  if (this->peer().set_option(IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt)) == -1) {
    ACE_ERROR((LM_ERROR, "Failed to set TCP_NODELAY\n"));
  }

  if (this->peer().set_option(SOL_SOCKET,
                              SO_SNDBUF,
                              (void *) &snd_size,
                              sizeof(snd_size)) == -1
      && errno != ENOTSUP) {
    ACE_ERROR((LM_ERROR,
               "(%P|%t) TcpConnection failed to set the send buffer size to %d errno %m\n",
               snd_size));
    return;
  }

  if (this->peer().set_option(SOL_SOCKET,
                              SO_RCVBUF,
                              (void *) &rcv_size,
                              sizeof(int)) == -1
      && errno != ENOTSUP) {
    ACE_ERROR((LM_ERROR,
               "(%P|%t) TcpConnection failed to set the receive buffer size to %d errno %m \n",
               rcv_size));
    return;
  }

#  else
  ACE_UNUSED_ARG(tcp_config);
  ACE_UNUSED_ARG(snd_size);
  ACE_UNUSED_ARG(rcv_size);
#  endif /* !ACE_LACKS_SOCKET_BUFSIZ */

#else
  ACE_UNUSED_ARG(tcp_config);
#endif /* !ACE_DEFAULT_MAX_SOCKET_BUFSIZ */
}

int
OpenDDS::DCPS::TcpConnection::active_establishment
(const ACE_INET_Addr& remote_address,
 const ACE_INET_Addr& local_address,
 TcpInst_rch tcp_config)
{
  DBG_ENTRY_LVL("TcpConnection","active_establishment",6);

  // Cache these values for reconnecting.
  this->remote_address_ = remote_address;
  this->local_address_ = local_address;
  this->tcp_config_ = tcp_config;

  /// @TODO: Priority is valid and this point and needs to be utilized.

  // Safty check - This should not happen since is_connector_ defaults to
  // true and the role in a connection connector is not changed when reconnecting.
  if (this->is_connector_ == false) {
    ACE_ERROR_RETURN((LM_ERROR,
                      "(%P|%t) ERROR: Failed to connect because it's previouly an acceptor.\n"),
                     -1);
  }

  if (this->shutdown_)
    return -1;

  // Now use a connector object to establish the connection.
  ACE_SOCK_Connector connector;

  if (connector.connect(this->peer(), remote_address) != 0) {
    std::ostringstream os;
    this->tcp_config_->dump(os);

    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: Failed to connect. %p\n%C"),
                      ACE_TEXT("connect"), os.str().c_str()),
                     -1);

  } else {
    this->connected_ = true;
    VDBG((LM_DEBUG, "(%P|%t) DBG:   "
      "active_establishment(%C:%d->%C:%d)\n",
      this->local_address_.get_host_addr(), this->local_address_.get_port_number(),
      this->remote_address_.get_host_addr(), this->remote_address_.get_port_number()));
  }

  // Set the DiffServ codepoint according to the priority value.
  DirectPriorityMapper mapper(this->transport_priority_);
  this->link_->set_dscp_codepoint(mapper.codepoint(), this->peer());

  set_sock_options(tcp_config.in());

  // In order to complete the connection establishment from the active
  // side, we need to tell the remote side about our local_address.
  // It will use that as an "identifier" of sorts.  To the other
  // (passive) side, our local_address that we send here will be known
  // as the remote_address.
  ACE_UINT32 len =
    static_cast<ACE_UINT32>(tcp_config_->local_address_str_.length()) + 1;

  ACE_UINT32 nlen = htonl(len);

  if (this->peer().send_n(&nlen,
                          sizeof(ACE_UINT32)) == -1) {
    // TBD later - Anything we are supposed to do to close the connection.
    ACE_ERROR_RETURN((LM_ERROR,
                      "(%P|%t) ERROR: Unable to send address string length to "
                      "the passive side to complete the active connection "
                      "establishment.\n"),
                     -1);
  }

  if (this->peer().send_n(tcp_config_->local_address_str_.c_str(),
                          len)  == -1) {
    // TBD later - Anything we are supposed to do to close the connection.
    ACE_ERROR_RETURN((LM_ERROR,
                      "(%P|%t) ERROR: Unable to send our address to "
                      "the passive side to complete the active connection "
                      "establishment.\n"),
                     -1);
  }

  ACE_UINT32 npriority = htonl(this->transport_priority_);

  if (this->peer().send_n(&npriority, sizeof(ACE_UINT32)) == -1) {
    // TBD later - Anything we are supposed to do to close the connection.
    ACE_ERROR_RETURN((LM_ERROR,
                      "(%P|%t) ERROR: Unable to send publication priority to "
                      "the passive side to complete the active connection "
                      "establishment.\n"),
                     -1);
  }

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
OpenDDS::DCPS::TcpConnection::reconnect(bool on_new_association)
{
  DBG_ENTRY_LVL("TcpConnection","reconnect",6);

  if (on_new_association)
    return this->active_reconnect_on_new_association();

  // If on_new_association is false, it's called by the reconnect task.
  // We need make sure if the link release is pending. If does, do
  // not try to reconnect.
  else if (! this->link_->is_release_pending ())
  {
    // Try to reconnect if it's connector previously.
    if (this->is_connector_ && this->active_reconnect_i() == -1)
      return -1;

    // Schedule a timer to see if a incoming connection is accepted when timeout.
    else if (!this->is_connector_ && this->passive_reconnect_i() == -1)
      return -1;
  }
  return 0;
}

int
OpenDDS::DCPS::TcpConnection::active_connect
(const ACE_INET_Addr& remote_address,
 const ACE_INET_Addr& local_address,
 CORBA::Long          priority,
 TcpInst_rch tcp_config)
{
  DBG_ENTRY_LVL("TcpConnection","active_connect",6);
  GuardType guard(this->reconnect_lock_);

  if (this->connected_ == true)
    return 0;

  this->transport_priority_ = priority;
  return this->active_establishment(remote_address,
                                    local_address,
                                    tcp_config);
}

int
OpenDDS::DCPS::TcpConnection::active_reconnect_on_new_association()
{
  DBG_ENTRY_LVL("TcpConnection","active_reconnect_on_new_association",6);
  GuardType guard(this->reconnect_lock_);

  if (this->connected_ == true)
    return 0;

  else if (this->active_establishment(this->remote_address_,
                                      this->local_address_,
                                      this->tcp_config_) == 0) {
    this->reconnect_state_ = INIT_STATE;
    this->send_strategy_->resume_send();
    return 0;
  }

  return -1;
}

// This method is called on acceptor side when the lost connection is detected.
// A timer is scheduled to check if a new connection is created within the
// passive_reconnect_duration_ period.
int
OpenDDS::DCPS::TcpConnection::passive_reconnect_i()
{
  DBG_ENTRY_LVL("TcpConnection","passive_reconnect_i",6);
  GuardType guard(this->reconnect_lock_);

  // The passive_reconnect_timer_id_ is used as flag to allow the timer scheduled just once.
  if (this->reconnect_state_ == INIT_STATE) {
    // Mark the connection lost since the recv/send just failed.
    this->connected_ = false;

    if (this->tcp_config_->passive_reconnect_duration_ == 0)
      return -1;

    ACE_Time_Value timeout(this->tcp_config_->passive_reconnect_duration_/1000,
                           this->tcp_config_->passive_reconnect_duration_%1000 * 1000);
    this->reconnect_state_ = PASSIVE_WAITING_STATE;
    this->link_->notify(DataLink::DISCONNECTED);

    // It is possible that the passive reconnect is called after the new connection
    // is accepted and the receive_strategy of this old connection is reset to nil.
    if (!this->receive_strategy_.is_nil()) {
      TcpReceiveStrategy* rs
      = dynamic_cast <TcpReceiveStrategy*>(this->receive_strategy_.in());

      // Give a copy to reactor.
      this->_add_ref();
      this->passive_reconnect_timer_id_ = rs->get_reactor()->schedule_timer(this, 0, timeout);

      if (this->passive_reconnect_timer_id_ == -1) {
        this->_remove_ref();
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: TcpConnection::passive_reconnect_i")
                          ACE_TEXT(", %p.\n"), ACE_TEXT("schedule_timer")),
                         -1);
      }
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
OpenDDS::DCPS::TcpConnection::active_reconnect_i()
{
  DBG_ENTRY_LVL("TcpConnection","active_reconnect_i",6);

  GuardType guard(this->reconnect_lock_);
  int ret = -1;

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
        "active_reconnect_i(%C:%d->%C:%d) reconnect_state = %d\n",
        this->remote_address_.get_host_addr(), this->remote_address_.get_port_number(),
        this->local_address_.get_host_addr(), this->local_address_.get_port_number(),
        this->reconnect_state_));

  // We need reset the state to INIT_STATE if we are previously reconnected.
  // This would allow re-establishing connection after the re-established
  // connection lost again.
  if (ACE_OS::gettimeofday() - this->last_reconnect_attempted_ > reconnect_delay
      && this->reconnect_state_ == RECONNECTED_STATE) {
    VDBG((LM_DEBUG, "(%P|%t) DBG:   "
          "We are in RECONNECTED_STATE and now flip reconnect state to INIT_STATE.\n"));
    this->reconnect_state_ = INIT_STATE;
  }

  if (this->reconnect_state_ == INIT_STATE) {
    // Suspend send once.
    this->send_strategy_->suspend_send();

    this->disconnect();

    if (this->tcp_config_->conn_retry_attempts_ > 0) {
      this->link_->notify(DataLink::DISCONNECTED);
    }

    // else the conn_retry_attempts is 0 then we do not need this extra
    // notify_disconnected() since the user application should get the
    // notify_lost() without delay.

    double retry_delay_msec = this->tcp_config_->conn_retry_initial_delay_;

    for (int i = 0; i < this->tcp_config_->conn_retry_attempts_; ++i) {
      ret = this->active_establishment(this->remote_address_,
                                       this->local_address_,
                                       this->tcp_config_);

      if (this->shutdown_)
        break;

      if (ret == -1) {
        ACE_Time_Value delay_tv(((int)retry_delay_msec)/1000,
                                ((int)retry_delay_msec)%1000*1000);
        ACE_OS::sleep(delay_tv);
        retry_delay_msec *= this->tcp_config_->conn_retry_backoff_multiplier_;

      } else {
        break;
      }
    }

    if (ret == -1) {
      if (this->tcp_config_->conn_retry_attempts_ > 0) {
        ACE_DEBUG((LM_DEBUG, "(%P|%t) we tried and failed to re-establish connection on transport: %C to %C:%d.\n",
                   this->link_->get_transport_impl()->get_transport_id_description().c_str(),
                   this->remote_address_.get_host_addr(),
                   this->remote_address_.get_port_number()));

      } else {
        ACE_DEBUG((LM_DEBUG, "(%P|%t) we did not try to re-establish connection on transport: %C to %C:%d.\n",
                   this->link_->get_transport_impl()->get_transport_id_description().c_str(),
                   this->remote_address_.get_host_addr(),
                   this->remote_address_.get_port_number()));
      }

      this->reconnect_state_ = LOST_STATE;
      this->link_->notify(DataLink::LOST);
      this->send_strategy_->terminate_send();

    } else {
      ACE_DEBUG((LM_DEBUG, "(%P|%t) re-established connection on transport: %C to %C:%d.\n",
                 this->link_->get_transport_impl()->get_transport_id_description().c_str(),
                 this->remote_address_.get_host_addr(),
                 this->remote_address_.get_port_number()));
      this->reconnect_state_ = RECONNECTED_STATE;
      this->link_->notify(DataLink::RECONNECTED);
      this->send_strategy_->resume_send();
    }

    this->last_reconnect_attempted_ = ACE_OS::gettimeofday();
  }

  return this->reconnect_state_ == LOST_STATE ? -1 : 0;
}

/// A timer is scheduled on acceptor side to check if a new connection
/// is accepted after the connection is lost.
int
OpenDDS::DCPS::TcpConnection::handle_timeout(const ACE_Time_Value &,
                                                   const void *)
{
  DBG_ENTRY_LVL("TcpConnection","handle_timeout",6);

  this->reconnect_state_ = PASSIVE_TIMEOUT_CALLED_STATE;
  GuardType guard(this->reconnect_lock_);

  switch (this->reconnect_state_) {
  case PASSIVE_TIMEOUT_CALLED_STATE: {
    // We stay in PASSIVE_TIMEOUT_CALLED_STATE indicates there is no new connection.
    // Now we need declare the connection is lost.
    this->link_->notify(DataLink::LOST);

    // The handle_timeout may be called after the connection is re-established
    // and the send strategy of this old connection is reset to nil.
    if (!this->send_strategy_.is_nil())
      this->send_strategy_->terminate_send();

    this->reconnect_state_ = LOST_STATE;
  }
  break;
  case RECONNECTED_STATE:
    // reconnected successfully.
    break;
  default :
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: TcpConnection::handle_timeout, ")
               ACE_TEXT(" unknown state or it should not be in state=%d \n"), this->reconnect_state_));
    break;
  }

  // Take back the "copy" we gave to reactor when we schedule the timer.
  this->_remove_ref();

  return 0;
}

/// This object would be "old" connection object and the provided is the new
/// connection object.  The "old" connection object will copy its states to
/// to the "new" connection object. This is called by the TcpDataLink
/// when a new connection is accepted (with a new TcpConnection object).
/// We need make the state in "new" connection object consistent with the "old"
/// connection object.
void
OpenDDS::DCPS::TcpConnection::transfer(TcpConnection* connection)
{
  DBG_ENTRY_LVL("TcpConnection","transfer",6);

  GuardType guard(this->reconnect_lock_);

  bool notify_reconnect = false;

  switch (this->reconnect_state_) {
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
  case PASSIVE_WAITING_STATE: {
    TcpReceiveStrategy* rs
    = dynamic_cast <TcpReceiveStrategy*>(this->receive_strategy_.in());

    // Cancel the timer since we got new connection.
    if (rs->get_reactor()->cancel_timer(this) == -1) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: TcpConnection::transfer, ")
                 ACE_TEXT(" %p. \n"), ACE_TEXT("cancel_timer")));

    } else
      passive_reconnect_timer_id_ = -1;

    this->_remove_ref();
    notify_reconnect = true;
  }
  break;
  default :
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: TcpConnection::transfer, ")
               ACE_TEXT(" unknown state or it should not be in state=%d \n"), this->reconnect_state_));
    break;
  }

  // Verify if this acceptor side.
  if (this->is_connector_ || connection->is_connector_) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: TcpConnection::transfer, ")
               ACE_TEXT(" should NOT be called by the connector side \n")));
  }

  this->reconnect_task_.close(1);
  connection->receive_strategy_ = this->receive_strategy_;
  connection->send_strategy_ = this->send_strategy_;
  connection->remote_address_ = this->remote_address_;
  connection->local_address_ = this->local_address_;
  connection->tcp_config_ = this->tcp_config_;
  connection->link_ = this->link_;

  //Make the "old" and "new" connection object keep a copy each other.
  //Note only does the "old" connection object call this transfer () function
  //since we need use the lock to synch this function and handle_timeout.
  connection->_add_ref();
  this->new_con_ = connection;

  this->_add_ref();
  connection->old_con_ = this;

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
        "transfer(%C:%d->%C:%d) passive reconnected. new con %X   "
        " old con %X \n",
        this->remote_address_.get_host_addr(), this->remote_address_.get_port_number(),
        this->local_address_.get_host_addr(), this->local_address_.get_port_number(),
        connection, this));

  if (notify_reconnect) {
    this->reconnect_state_ = RECONNECTED_STATE;
    this->link_->notify(DataLink::RECONNECTED);
  }
}

/// This function is called when the backpresure occurs and timed out after
/// "max_output_pause_period". The lost connection notification should be sent
/// and the connection needs be closed since we declared it as a "lost"
/// connection.
void
OpenDDS::DCPS::TcpConnection::notify_lost_on_backpressure_timeout()
{
  DBG_ENTRY_LVL("TcpConnection","notify_lost_on_backpressure_timeout",6);
  bool notify_lost = false;
  {
    GuardType guard(this->reconnect_lock_);

    if (this->reconnect_state_ == INIT_STATE) {
      this->reconnect_state_ = LOST_STATE;
      notify_lost = true;

      this->disconnect();
    }
  }

  if (notify_lost) {
    this->link_->notify(DataLink::LOST);
    this->send_strategy_->terminate_send();
  }
}

/// This is called by both TcpSendStrategy and TcpReceiveStrategy
/// when lost connection is detected. This method handles the connection
/// to the reactor task to do the reconnecting.
void
OpenDDS::DCPS::TcpConnection::relink(bool do_suspend)
{
  DBG_ENTRY_LVL("TcpConnection","relink",6);

  if (do_suspend && !this->send_strategy_.is_nil())
    this->send_strategy_->suspend_send();

  ReconnectOpType op = DO_RECONNECT;
  this->reconnect_task_.add(op);
}

bool
OpenDDS::DCPS::TcpConnection::tear_link()
{
  DBG_ENTRY_LVL("TcpConnection","tear_link",6);

  return this->link_->release_resources();
}

void
OpenDDS::DCPS::TcpConnection::shutdown()
{
  DBG_ENTRY_LVL("TcpConnection","shutdown",6);
  this->shutdown_ = true;

  this->reconnect_task_.close(1);
}
