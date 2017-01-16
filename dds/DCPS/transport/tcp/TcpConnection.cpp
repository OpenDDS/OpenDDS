/*
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
#include "dds/DCPS/transport/framework/PriorityKey.h"

#include "ace/os_include/netinet/os_tcp.h"
#include "ace/OS_NS_arpa_inet.h"
#include "ace/OS_NS_unistd.h"
#include <sstream>
#include <string>

#if !defined (__ACE_INLINE__)
#include "TcpConnection.inl"
#endif /* __ACE_INLINE__ */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

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
  : connected_(false)
  , is_connector_(false)
  , passive_reconnect_timer_id_(-1)
  , reconnect_task_(this)
  , reconnect_state_(INIT_STATE)
  , last_reconnect_attempted_(ACE_Time_Value::zero)
  , transport_priority_(0)  // TRANSPORT_PRIORITY.value default value - 0.
  , shutdown_(false)
  , passive_setup_(false)
  , passive_setup_buffer_(sizeof(ACE_UINT32))
  , id_(0)
{
  DBG_ENTRY_LVL("TcpConnection","TcpConnection",6);

  this->reference_counting_policy().value(ACE_Event_Handler::Reference_Counting_Policy::ENABLED);

  if (this->reconnect_task_.open()) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: Reconnect task failed to open : %p\n"),
               ACE_TEXT("open")));
  }

}

OpenDDS::DCPS::TcpConnection::TcpConnection(const ACE_INET_Addr& remote_address,
                                            Priority priority,
                                            const TcpInst_rch& config)
  : connected_(false)
  , is_connector_(true)
  , remote_address_(remote_address)
  , local_address_(config->local_address())
  , tcp_config_(config)
  , passive_reconnect_timer_id_(-1)
  , reconnect_task_(this)
  , reconnect_state_(INIT_STATE)
  , last_reconnect_attempted_(ACE_Time_Value::zero)
  , transport_priority_(priority)
  , shutdown_(false)
  , passive_setup_(false)
  , id_(0)
{
  DBG_ENTRY_LVL("TcpConnection","TcpConnection",6);
  this->reference_counting_policy().value(ACE_Event_Handler::Reference_Counting_Policy::ENABLED);

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

  // The Reconnect task belongs to the Connection object.
  // Cleanup before leaving the house.
  this->reconnect_task_.close(1);
  //this->reconnect_task_.wait ();

  if (!this->link_.is_nil()) {
    if (Transport_debug_level > 5) {
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) TcpConnection::~TcpConnection: about to notify link[%@] connection deleted\n"),
                 this->link_.in()));
    }
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
(const TcpReceiveStrategy_rch& receive_strategy)
{
  DBG_ENTRY_LVL("TcpConnection","set_receive_strategy",6);

  // Make a "copy" for ourselves
  this->receive_strategy_ = receive_strategy;
}

void
OpenDDS::DCPS::TcpConnection::set_send_strategy
(const TcpSendStrategy_rch& send_strategy)
{
  DBG_ENTRY_LVL("TcpConnection","set_send_strategy",6);

  // Make a "copy" for ourselves
  this->send_strategy_ = send_strategy;
}

int
OpenDDS::DCPS::TcpConnection::open(void* arg)
{
  DBG_ENTRY_LVL("TcpConnection","open",6);

  if (is_connector_) {

    VDBG_LVL((LM_DEBUG, "(%P|%t) DBG:   TcpConnection::open active.\n"), 2);
    // Take over the refcount from TcpTransport::connect_datalink().
    const TcpConnection_rch self(this, keep_count());
    const TcpTransport_rch transport = link_->get_transport_impl();

    const bool is_loop(local_address_ == remote_address_);
    const PriorityKey key(transport_priority_, remote_address_,
                          is_loop, false /* !active */);

    int active_open_ = active_open();

    int connect_tcp_datalink_ = transport->connect_tcp_datalink(link_, self);

    if (active_open_ == -1 || connect_tcp_datalink_ == -1) {
      // if (active_open() == -1 ||
      //       transport->connect_tcp_datalink(link_, self) == -1) {

      transport->async_connect_failed(key);

      return -1;
    }

    return 0;
  }

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
  TcpTransport_rch transport(acceptor->transport());

  if (transport.is_nil()) {
    // The acceptor gave us a nil transport (smart) pointer.
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: TcpConnection::open() - ")
                      ACE_TEXT("acceptor's transport is nil.\n")),
                     -1);
  }

  // Keep a "copy" of the reference to TcpInst object
  // for ourselves.
  tcp_config_ =  acceptor->get_configuration();
  local_address_ = tcp_config_->local_address();

  set_sock_options(tcp_config_.in());

  // We expect that the active side of the connection (the remote side
  // in this case) will supply its listening ACE_INET_Addr as the first
  // message it sends to the socket.  This is a one-way connection
  // establishment protocol message.
  passive_setup_ = true;
  transport_during_setup_ = transport;
  passive_setup_buffer_.size(sizeof(ACE_UINT32));

  if (reactor()->register_handler(this, READ_MASK) == -1) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: TcpConnection::open() - ")
                      ACE_TEXT("unable to register with the reactor.%p\n"),
                      ACE_TEXT("register_handler")),
                     -1);
  }

  VDBG_LVL((LM_DEBUG, "(%P|%t) DBG:   TcpConnection::open passive handle=%d.\n",
            static_cast<int>(intptr_t(get_handle()))), 2);

  return 0;
}

int
OpenDDS::DCPS::TcpConnection::handle_setup_input(ACE_HANDLE /*h*/)
{
  const ssize_t ret = peer().recv(passive_setup_buffer_.wr_ptr(),
                                  passive_setup_buffer_.space(),
                                  &ACE_Time_Value::zero);

  if (ret < 0 && errno == ETIME) {
    return 0;
  }

  VDBG_LVL((LM_DEBUG, "(%P|%t) DBG:   TcpConnection::handle_setup_input %@ "
            "recv returned %b %m.\n", this, ret), 4);

  if (ret <= 0) {
    return -1;
  }

  passive_setup_buffer_.wr_ptr(ret);
  // Parse the setup message: <len><addr><prio>
  // len and prio are network order 32-bit ints
  // addr is a string of length len, including null
  ACE_UINT32 nlen = 0;

  if (passive_setup_buffer_.length() >= sizeof(nlen)) {

    ACE_OS::memcpy(&nlen, passive_setup_buffer_.rd_ptr(), sizeof(nlen));
    passive_setup_buffer_.rd_ptr(sizeof(nlen));
    ACE_UINT32 hlen = ntohl(nlen);
    passive_setup_buffer_.size(hlen + 2 * sizeof(nlen));

    ACE_UINT32 nprio = 0;

    if (passive_setup_buffer_.length() >= hlen + sizeof(nprio)) {

      const std::string bufstr(passive_setup_buffer_.rd_ptr());
      const NetworkAddress network_order_address(bufstr);
      network_order_address.to_addr(remote_address_);

      ACE_OS::memcpy(&nprio, passive_setup_buffer_.rd_ptr() + hlen, sizeof(nprio));
      transport_priority_ = ntohl(nprio);

      passive_setup_buffer_.reset();
      passive_setup_ = false;

      VDBG((LM_DEBUG, "(%P|%t) DBG:   TcpConnection::handle_setup_input "
            "%@ %C:%d->%C:%d, priority==%d, reconnect_state = %d\n", this,
            remote_address_.get_host_addr(), remote_address_.get_port_number(),
            local_address_.get_host_addr(), local_address_.get_port_number(),
            transport_priority_, reconnect_state_));

      // remove from reactor, normal recv strategy setup will add us back
      if (reactor()->remove_handler(this, READ_MASK | DONT_CALL) == -1) {
        VDBG((LM_DEBUG, "(%P|%t) DBG:   TcpConnection::handle_setup_input "
              "remove_handler failed %m.\n"));
      }

      transport_during_setup_->passive_connection(remote_address_, rchandle_from(this));
      transport_during_setup_.reset();
      connected_ = true;

      return 0;
    }
  }

  passive_setup_buffer_.rd_ptr(passive_setup_buffer_.base());

  return 0;
}

int
OpenDDS::DCPS::TcpConnection::handle_input(ACE_HANDLE fd)
{
  DBG_ENTRY_LVL("TcpConnection","handle_input",6);

  if (passive_setup_) {
    return handle_setup_input(fd);
  }

  if (receive_strategy_.is_nil()) {
    return 0;
  }

  return receive_strategy_->handle_dds_input(fd);
}

int
OpenDDS::DCPS::TcpConnection::handle_output(ACE_HANDLE)
{
  DBG_ENTRY_LVL("TcpConnection","handle_output",6);

  if (!this->send_strategy_.is_nil()) {
    if (DCPS_debug_level > 9) {
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) TcpConnection::handle_output() [%d] - ")
                 ACE_TEXT("sending queued data.\n"),
                 id_));
    }

    // Process data to be sent from the queue.
    if (ThreadSynchWorker::WORK_OUTCOME_MORE_TO_DO
        != send_strategy_->perform_work()) {

      // Stop handling output ready events when there is nothing to output.
      // N.B. This calls back into the reactor.  Is the reactor lock
      //      recursive?
      send_strategy_->schedule_output();
    }
  }

  return 0;
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

  if (DCPS_debug_level >= 1) {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) TcpConnection::handle_close() called on transport: %C to %C:%d.\n",
               this->link_->get_transport_impl()->config()->name().c_str(),
               this->remote_address_.get_host_addr(),
               this->remote_address_.get_port_number()));
  }

  bool graceful = !this->receive_strategy_.is_nil() && this->receive_strategy_->gracefully_disconnected();

  if (!this->send_strategy_.is_nil()) {
    if (graceful) {
      this->send_strategy_->terminate_send();
    } else {
      this->send_strategy_->suspend_send();
    }
  }

  this->disconnect();

  if (graceful) {
    this->link_->notify(DataLink::DISCONNECTED);
  } else {
    ReconnectOpType op = DO_RECONNECT;
    this->reconnect_task_.add(op);
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
OpenDDS::DCPS::TcpConnection::active_establishment(bool initiate_connect)
{
  DBG_ENTRY_LVL("TcpConnection","active_establishment",6);

  // Safety check - This should not happen since is_connector_ defaults to
  // true and the role in a connection connector is not changed when reconnecting.
  if (this->is_connector_ == false) {
    ACE_ERROR_RETURN((LM_ERROR,
                      "(%P|%t) ERROR: Failed to connect because it's previously an acceptor.\n"),
                     -1);
  }

  if (this->shutdown_)
    return -1;

  // Now use a connector object to establish the connection.
  ACE_SOCK_Connector connector;

  if (initiate_connect && connector.connect(this->peer(), remote_address_) != 0) {

    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: Failed to connect. %p\n%C"),
                      ACE_TEXT("connect"), this->tcp_config_->dump_to_str().c_str()),
                     -1);

  } else {
    this->connected_ = true;
    const std::string remote_host = this->remote_address_.get_host_addr();
    VDBG((LM_DEBUG, "(%P|%t) DBG:   active_establishment(%C:%d->%C:%d)\n",
          this->local_address_.get_host_addr(), this->local_address_.get_port_number(),
          remote_host.c_str(), this->remote_address_.get_port_number()));
  }

  // Set the DiffServ codepoint according to the priority value.
  DirectPriorityMapper mapper(this->transport_priority_);
  this->link_->set_dscp_codepoint(mapper.codepoint(), this->peer());

  set_sock_options(tcp_config_.in());

  // In order to complete the connection establishment from the active
  // side, we need to tell the remote side about our public address.
  // It will use that as an "identifier" of sorts.  To the other
  // (passive) side, our local_address that we send here will be known
  // as the remote_address.
  std::string address = tcp_config_->get_public_address();
  ACE_UINT32 len = static_cast<ACE_UINT32>(address.length()) + 1;

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

  if (this->peer().send_n(address.c_str(), len)  == -1) {
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
  if (DCPS_debug_level >= 1) {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) TcpConnection::reconnect initiated on transport: %C to %C:%d.\n",
               this->link_->get_transport_impl()->config()->name().c_str(),
               this->remote_address_.get_host_addr(),
               this->remote_address_.get_port_number()));
  }

  if (on_new_association) {
    if (DCPS_debug_level >= 1) {
      ACE_DEBUG((LM_DEBUG, "(%P|%t) TcpConnection::reconnect on new association\n"));
    }
    return this->active_reconnect_on_new_association();
  }

  // If on_new_association is false, it's called by the reconnect task.
  // We need make sure if the link release is pending. If does, do
  // not try to reconnect.
  else if (!this->link_->is_release_pending()) {
    if (DCPS_debug_level >= 1) {
      ACE_DEBUG((LM_DEBUG, "(%P|%t) TcpConnection::reconnect release not currently pending\n"));
    }
    // Try to reconnect if it's connector previously.
    if (this->is_connector_ && this->active_reconnect_i() == -1) {
      if (DCPS_debug_level >= 1) {
        ACE_DEBUG((LM_DEBUG, "(%P|%t) TcpConnection::reconnect is connector but active_reconnect_i failed\n"));
      }
      return -1;
    }

    // Schedule a timer to see if a incoming connection is accepted when timeout.
    else if (!this->is_connector_ && this->passive_reconnect_i() == -1) {
      if (DCPS_debug_level >= 1) {
        ACE_DEBUG((LM_DEBUG, "(%P|%t) TcpConnection::reconnect is acceptor but passive_reconnect_i failed\n"));
      }
      return -1;
    }

  }
  if (DCPS_debug_level >= 1) {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) TcpConnection::reconnect returning 0\n"));
  }
  return 0;
}

int
OpenDDS::DCPS::TcpConnection::active_open()
{
  DBG_ENTRY_LVL("TcpConnection","active_open",6);

  GuardType guard(reconnect_lock_);

  if (connected_.value()) {
    return 0;
  }

  return active_establishment(false /* !initiate_connect */);
}

int
OpenDDS::DCPS::TcpConnection::active_reconnect_on_new_association()
{
  DBG_ENTRY_LVL("TcpConnection","active_reconnect_on_new_association",6);
  GuardType guard(this->reconnect_lock_);

  if (this->connected_ == true)
    return 0;

  else if (this->active_establishment() == 0) {
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
      this->passive_reconnect_timer_id_ = rs->get_reactor()->schedule_timer(this, 0, timeout);

      if (this->passive_reconnect_timer_id_ == -1) {
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

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
        "active_reconnect_i(%C:%d->%C:%d) reconnect_state = %d\n",
        this->remote_address_.get_host_addr(), this->remote_address_.get_port_number(),
        this->local_address_.get_host_addr(), this->local_address_.get_port_number(),
        this->reconnect_state_));
  if (DCPS_debug_level >= 1) {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) DBG:   TcpConnection::"
          "active_reconnect_i(%C:%d->%C:%d) reconnect_state = %d\n",
          this->remote_address_.get_host_addr(), this->remote_address_.get_port_number(),
          this->local_address_.get_host_addr(), this->local_address_.get_port_number(),
          this->reconnect_state_));
  }
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
    int ret = -1;

    for (int i = 0; i < this->tcp_config_->conn_retry_attempts_; ++i) {
      ret = this->active_establishment();

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
                   this->link_->get_transport_impl()->config()->name().c_str(),
                   this->remote_address_.get_host_addr(),
                   this->remote_address_.get_port_number()));

      } else {
        ACE_DEBUG((LM_DEBUG, "(%P|%t) we did not try to re-establish connection on transport: %C to %C:%d.\n",
                   this->link_->get_transport_impl()->config()->name().c_str(),
                   this->remote_address_.get_host_addr(),
                   this->remote_address_.get_port_number()));
      }

      this->reconnect_state_ = LOST_STATE;
      this->link_->notify(DataLink::LOST);
      this->send_strategy_->terminate_send();

    } else {
      ACE_DEBUG((LM_DEBUG, "(%P|%t) re-established connection on transport: %C to %C:%d.\n",
                 this->link_->get_transport_impl()->config()->name().c_str(),
                 this->remote_address_.get_host_addr(),
                 this->remote_address_.get_port_number()));
      if (this->receive_strategy_->get_reactor()->register_handler(this, ACE_Event_Handler::READ_MASK) == -1) {
        ACE_ERROR_RETURN((LM_ERROR,
                          "(%P|%t) ERROR: OpenDDS::DCPS::TcpConnection::active_reconnect_i() can't register "
                          "with reactor %X %p\n", this, ACE_TEXT("register_handler")),
                         -1);
      }
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

  GuardType guard(this->reconnect_lock_);

  switch (this->reconnect_state_) {
  case PASSIVE_WAITING_STATE: {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) TcpConnection::handle_timeout, we tried and failed to re-establish connection on transport: %C to %C:%d.\n",
               this->link_->get_transport_impl()->config()->name().c_str(),
               this->remote_address_.get_host_addr(),
               this->remote_address_.get_port_number()));

    this->reconnect_state_ = PASSIVE_TIMEOUT_CALLED_STATE;
    // We stay in PASSIVE_TIMEOUT_CALLED_STATE indicates there is no new connection.
    // Now we need declare the connection is lost.
    this->link_->notify(DataLink::LOST);

    // The handle_timeout may be called after the connection is re-established
    // and the send strategy of this old connection is reset to nil.
    if (!this->send_strategy_.is_nil())
      this->send_strategy_->terminate_send();

    this->reconnect_state_ = LOST_STATE;

    this->tear_link();

  }
  break;

  case RECONNECTED_STATE:
    // reconnected successfully.
    ACE_DEBUG((LM_DEBUG, "(%P|%t) TcpConnection::handle_timeout, re-established connection on transport: %C to %C:%d.\n",
               this->link_->get_transport_impl()->config()->name().c_str(),
               this->remote_address_.get_host_addr(),
               this->remote_address_.get_port_number()));
    break;

  default :
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: TcpConnection::handle_timeout, ")
               ACE_TEXT(" unknown state or it should not be in state=%d \n"), this->reconnect_state_));
    break;
  }

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

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
        "transfer(%C:%d->%C:%d) passive reconnected. new con %@   "
        " old con %@ \n",
        this->remote_address_.get_host_addr(), this->remote_address_.get_port_number(),
        this->local_address_.get_host_addr(), this->local_address_.get_port_number(),
        connection, this));

  if (notify_reconnect) {
    this->reconnect_state_ = RECONNECTED_STATE;
    this->link_->notify(DataLink::RECONNECTED);
  }

}

/// This function is called when the backpressure occurs and timed out after
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

/// This is called by TcpSendStrategy when a send fails
/// and a reconnect should be initiated. This method
/// suspends any sends and kicks the reconnect thread into
/// action.
void
OpenDDS::DCPS::TcpConnection::relink_from_send(bool do_suspend)
{
  DBG_ENTRY_LVL("TcpConnection","relink_from_send",6);

  if (do_suspend && !this->send_strategy_.is_nil())
    this->send_strategy_->suspend_send();

  ReconnectOpType op = DO_RECONNECT;
  this->reconnect_task_.add(op);
}

/// This is called by TcpReceiveStrategy when a disconnect
/// is detected.  It simply suspends any sends and lets
/// the handle_close() handle the reconnect logic.
void
OpenDDS::DCPS::TcpConnection::relink_from_recv(bool do_suspend)
{
  DBG_ENTRY_LVL("TcpConnection","relink_from_recv",6);

  if (do_suspend && !this->send_strategy_.is_nil())
    this->send_strategy_->suspend_send();
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

ACE_Event_Handler::Reference_Count
OpenDDS::DCPS::TcpConnection::add_reference()
{
  RcObject<ACE_SYNCH_MUTEX>::_add_ref();
  return 1;
}

ACE_Event_Handler::Reference_Count
OpenDDS::DCPS::TcpConnection::remove_reference()
{
  RcObject<ACE_SYNCH_MUTEX>::_remove_ref();
  return 1;
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
