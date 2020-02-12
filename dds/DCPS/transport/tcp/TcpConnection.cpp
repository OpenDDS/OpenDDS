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
#include "dds/DCPS/transport/framework/DirectPriorityMapper.h"
#include "dds/DCPS/transport/framework/PriorityKey.h"
#include "dds/DCPS/Time_Helper.h"

using ::OpenDDS::DCPS::MonotonicTimePoint;
using ::OpenDDS::DCPS::TimeDuration;

#include "ace/os_include/netinet/os_tcp.h"
#include "ace/OS_NS_arpa_inet.h"
#include "ace/OS_NS_unistd.h"
#include <sstream>
#include <string>
#include <cmath>

#if !defined (__ACE_INLINE__)
#include "TcpConnection.inl"
#endif /* __ACE_INLINE__ */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

OpenDDS::DCPS::TcpConnection::TcpConnection()
  : is_connector_(false)
  , tcp_config_(0)
  , reconnect_state_(INIT_STATE)
  , transport_priority_(0)  // TRANSPORT_PRIORITY.value default value - 0.
  , shutdown_(false)
  , passive_setup_(false)
  , passive_setup_buffer_(sizeof(ACE_UINT32))
  , transport_during_setup_(0)
  , id_(0)
  , conn_retry_counter_(0)
{
  DBG_ENTRY_LVL("TcpConnection","TcpConnection",6);
  this->reference_counting_policy().value(ACE_Event_Handler::Reference_Counting_Policy::ENABLED);
}

OpenDDS::DCPS::TcpConnection::TcpConnection(const ACE_INET_Addr& remote_address,
                                            Priority priority,
                                            const TcpInst& config)
  : is_connector_(true)
  , remote_address_(remote_address)
  , local_address_(config.local_address())
  , tcp_config_(&config)
  , reconnect_state_(INIT_STATE)
  , transport_priority_(priority)
  , shutdown_(false)
  , passive_setup_(false)
  , transport_during_setup_(0)
  , id_(0)
  , conn_retry_counter_(0)
{
  DBG_ENTRY_LVL("TcpConnection","TcpConnection",6);
  this->reference_counting_policy().value(ACE_Event_Handler::Reference_Counting_Policy::ENABLED);
}

OpenDDS::DCPS::TcpConnection::~TcpConnection()
{
  DBG_ENTRY_LVL("TcpConnection","~TcpConnection",6);
  shutdown();
}

void
OpenDDS::DCPS::TcpConnection::set_datalink(const OpenDDS::DCPS::TcpDataLink_rch& link)
{
  DBG_ENTRY_LVL("TcpConnection","set_datalink",6);

  GuardType guard(reconnect_lock_);

  link_ = link;
  if (link_) {
    impl_ = TcpTransport_rch(static_cast<TcpTransport*>(&link_->impl()), inc_count());
  } else {
    impl_.reset();
  }
}


OpenDDS::DCPS::TcpSendStrategy_rch
OpenDDS::DCPS::TcpConnection::send_strategy()
{
  return this->link_->send_strategy();
}

OpenDDS::DCPS::TcpReceiveStrategy_rch
OpenDDS::DCPS::TcpConnection::receive_strategy()
{
  return this->link_->receive_strategy();
}

void
OpenDDS::DCPS::TcpConnection::disconnect()
{
  DBG_ENTRY_LVL("TcpConnection","disconnect",6);

  if (this->link_) {
    this->link_->drop_pending_request_acks();
  }

  this->peer().close();
}

int
OpenDDS::DCPS::TcpConnection::open(void* arg)
{
  DBG_ENTRY_LVL("TcpConnection","open",6);

  if (is_connector_) {
    if (this->reconnect_state_ == ACTIVE_RECONNECTING_STATE) {
      return active_reconnect_open();
    }
    return active_open();
  }
  return passive_open(arg);
}

int
OpenDDS::DCPS::TcpConnection::active_open()
{
  VDBG_LVL((LM_DEBUG, "(%P|%t) DBG:   TcpConnection::active_open.\n"), 2);

  const std::string remote_host = this->remote_address_.get_host_addr();
  VDBG((LM_DEBUG, "(%P|%t) DBG:   active_open(%C:%d->%C:%d)\n",
        this->local_address_.get_host_addr(), this->local_address_.get_port_number(),
        remote_host.c_str(), this->remote_address_.get_port_number()));

  TcpTransport& transport = static_cast<TcpTransport&>(link_->impl());

  if (on_active_connection_established() != -1 && transport.connect_tcp_datalink(*link_, rchandle_from(this)) != -1)
    return 0;

  const bool is_loop(local_address_ == remote_address_);
  const PriorityKey key(transport_priority_, remote_address_,
                      is_loop, true /* active */);
  transport.async_connect_failed(key);
  return -1;
}

int
OpenDDS::DCPS::TcpConnection::passive_open(void* arg)
{
  VDBG_LVL((LM_DEBUG, "(%P|%t) DBG:   TcpConnection::passive_open.\n"), 2);
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

  TcpConnection_rch self(this, keep_count());

  // Now we need to ask the TcpAcceptor object to provide us with
  // a pointer to the TcpTransport object that "owns" the acceptor.
  TcpTransport* transport = acceptor->transport();

  if (!transport) {
    // The acceptor gave us a nil transport (smart) pointer.
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: TcpConnection::open() - ")
                      ACE_TEXT("acceptor's transport is nil.\n")),
                     -1);
  }

  // Keep a "copy" of the reference to TcpInst object
  // for ourselves.
  tcp_config_ =  &acceptor->get_configuration();
  local_address_ = tcp_config_->local_address();

  set_sock_options(tcp_config_);

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
            "%@ %C:%d->%C:%d, priority==%d, reconnect_state = %C\n", this,
            remote_address_.get_host_addr(), remote_address_.get_port_number(),
            local_address_.get_host_addr(), local_address_.get_port_number(),
            transport_priority_, reconnect_state_string()));

      // remove from reactor, normal recv strategy setup will add us back
      if (reactor()->remove_handler(this, READ_MASK | DONT_CALL) == -1) {
        VDBG((LM_DEBUG, "(%P|%t) DBG:   TcpConnection::handle_setup_input "
              "remove_handler failed %m.\n"));
      }

      transport_during_setup_->passive_connection(remote_address_, rchandle_from(this));

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
  TcpReceiveStrategy_rch receive_strategy = this->receive_strategy();
  if (!receive_strategy) {
    return 0;
  }

  return receive_strategy->handle_dds_input(fd);
}

int
OpenDDS::DCPS::TcpConnection::handle_output(ACE_HANDLE)
{
  DBG_ENTRY_LVL("TcpConnection","handle_output",6);
  TcpSendStrategy_rch send_strategy = this->send_strategy();
  if (send_strategy) {
    if (DCPS_debug_level > 9) {
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) TcpConnection::handle_output() [%d] - ")
                 ACE_TEXT("sending queued data.\n"),
                 id_));
    }

    // Process data to be sent from the queue.
    if (ThreadSynchWorker::WORK_OUTCOME_MORE_TO_DO
        != send_strategy->perform_work()) {

      // Stop handling output ready events when there is nothing to output.
      // N.B. This calls back into the reactor.  Is the reactor lock
      //      recursive?
      send_strategy->schedule_output();
    }
  }

  return 0;
}

int
OpenDDS::DCPS::TcpConnection::close(u_long)
{
  DBG_ENTRY_LVL("TcpConnection","close",6);

  ACE_DEBUG((LM_DEBUG, "TcpConnection::close, reconnect_state_=%C\n", reconnect_state_string()));

  if (this->reconnect_state_ == ACTIVE_RECONNECTING_STATE) {
    // This would be called when using ACE_Connector to initiate an async connect and
    // the network stack detects the destination is unreachable before timeout.
    if (DCPS_debug_level >= 1) {
      ACE_DEBUG((LM_DEBUG, "(%P|%t) TcpConnection()::close() on transport: %C to %C:%d because of reconnect failure.\n",
            this->config_name().c_str(),
            this->remote_address_.get_host_addr(),
            this->remote_address_.get_port_number()));
    }

    if (this->conn_retry_counter_ >= this->tcp_config_->conn_retry_attempts_) {
      this->handle_stop_reconnecting();
    } else {
      TcpTransport& transport = static_cast<TcpTransport&>(link_->impl());
      transport.connector_.close();

      this->reconnect_state_ = ACTIVE_WAITING_STATE;
    }
  } else {
    TcpSendStrategy_rch send_strategy = this->send_strategy();
    if (send_strategy)
      send_strategy->terminate_send();

    this->disconnect();
  }

  return 0;
}

const std::string&
OpenDDS::DCPS::TcpConnection::config_name() const
{
  return impl_->config().name();
}

int
OpenDDS::DCPS::TcpConnection::handle_close(ACE_HANDLE, ACE_Reactor_Mask)
{
  DBG_ENTRY_LVL("TcpConnection","handle_close",6);

  if (DCPS_debug_level >= 1) {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) TcpConnection::handle_close() called on transport: %C to %C:%d , reconnect_state = %C.\n",
               this->config_name().c_str(),
               this->remote_address_.get_host_addr(),
               this->remote_address_.get_port_number(), reconnect_state_string()));
  }

  GuardType guard(reconnect_lock_);
  TcpDataLink_rch link = link_;


  if (!link) {
    if (DCPS_debug_level >= 1) {
      ACE_DEBUG((LM_DEBUG, "(%P|%t) TcpConnection::handle_close() link is null.\n"));
    }
    return 0;
  }

  TcpReceiveStrategy_rch receive_strategy = link->receive_strategy();
  TcpSendStrategy_rch send_strategy = link->send_strategy();

  bool graceful = receive_strategy && receive_strategy->gracefully_disconnected();

  if (send_strategy) {
    if (graceful) {
      send_strategy->terminate_send();
    } else {
      send_strategy->suspend_send();
    }
  }

  this->disconnect();

  if (graceful) {
    link->notify(DataLink::DISCONNECTED);
  } else if (this->is_connector_) {
    this->active_reconnect_i();
  } else {
    this->passive_reconnect_i();
  }

  return 0;
}

void
OpenDDS::DCPS::TcpConnection::set_sock_options(const TcpInst* tcp_config)
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
OpenDDS::DCPS::TcpConnection::on_active_connection_established()
{

  // Set the DiffServ codepoint according to the priority value.
  DirectPriorityMapper mapper(this->transport_priority_);
  this->link_->set_dscp_codepoint(mapper.codepoint(), this->peer());

  set_sock_options(tcp_config_);

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

// This method is called on acceptor side when the lost connection is detected.
// A timer is scheduled to check if a new connection is created within the
// passive_reconnect_duration_ period.
void
OpenDDS::DCPS::TcpConnection::passive_reconnect_i()
{
  DBG_ENTRY_LVL("TcpConnection","passive_reconnect_i",6);

  if (this->shutdown_)
    return;

  if (this->reconnect_state_ == INIT_STATE) {
    // Mark the connection lost since the recv/send just failed.
    // this->connected_ = false;

    if (this->tcp_config_->passive_reconnect_duration_ == 0)
      return;

    this->reconnect_state_ = PASSIVE_WAITING_STATE;
    this->link_->notify(DataLink::DISCONNECTED);

    TimeDuration delay = TimeDuration::from_msec(tcp_config_->passive_reconnect_duration_);
    this->reactor()->schedule_timer(this, 0, delay.value(), TimeDuration::zero_value.value());
  }
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
void
OpenDDS::DCPS::TcpConnection::active_reconnect_i()
{
  DBG_ENTRY_LVL("TcpConnection","active_reconnect_i",6);

  if (this->link_->is_release_pending())
    return;

  if (this->shutdown_)
    return;

  if (this->conn_retry_counter_ < this->tcp_config_->conn_retry_attempts_ ) {
    this->reconnect_state_ = ACTIVE_RECONNECTING_STATE;
    if (this->conn_retry_counter_ == 0)
      this->link_->notify(DataLink::DISCONNECTED);

    double retry_delay_msec = this->tcp_config_->conn_retry_initial_delay_;
    retry_delay_msec *= std::pow(this->tcp_config_->conn_retry_backoff_multiplier_, this->conn_retry_counter_);

    if (DCPS_debug_level >= 1) {
      ACE_DEBUG((LM_DEBUG, "(%P|%t) DBG:   TcpConnection::"
            "active_reconnect_i(%C:%d->%C:%d) reconnect_state = %C, conn_retry_counter_=%d, retry_delay_msec=%f\n",
            remote_address_.get_host_addr(), remote_address_.get_port_number(),
            local_address_.get_host_addr(), local_address_.get_port_number(),
            reconnect_state_string(), this->conn_retry_counter_, retry_delay_msec));
    }

    TcpTransport& transport = static_cast<TcpTransport&>(link_->impl());
    ACE_Time_Value timeout;
    timeout.msec(static_cast<int>(retry_delay_msec));

    TcpConnection* pconn = this;
    int ret;
    {
      ACE_Reverse_Lock<LockType> rev_lock(this->reconnect_lock_);
      ACE_Guard<ACE_Reverse_Lock<LockType> > guard(rev_lock);
      // We need to temporarily release the lock here because the connect could occasionally be synchronous
      // if the source and destination are on the same host. When the call become synchronous, active_reconnect_open()
      // would be called and try to acquired the lock in the same thread.
      ret = transport.connector_.connect(pconn, this->remote_address_,  ACE_Synch_Options::asynch);
    }

    if (ret == -1 && errno != EWOULDBLOCK)
    {
      if (errno == EALREADY) {
        // This could happen on Windows, it may due to the close() on non-blocking socket needs more time to complete.
        // In this case, we just wait another second to initiate the connect again without incrementing the conn_retry_counter_.
        timeout.sec(1);
        --conn_retry_counter_;
        if (DCPS_debug_level >= 1) {
          ACE_DEBUG((LM_DEBUG, "(%P|%t) DBG:   TcpConnection::"
                               "active_reconnect_i() socket operation is already in progress, wait another second to initiate the connect\n"));
        }
      }
      else {
        ACE_ERROR((LM_ERROR, "(%P|%t) TcpConnection::active_reconnect_i error %m.\n"));
      }
      this->reconnect_state_ = ACTIVE_WAITING_STATE;
    }

    this->reactor()->schedule_timer(this, 0, timeout);
    this->conn_retry_counter_ ++;
  }
  else {
    this->handle_stop_reconnecting();
  }
}

void
OpenDDS::DCPS::TcpConnection::notify_connection_lost()
{
  if (this->link_) {
    this->link_->notify(DataLink::LOST);
    TcpSendStrategy_rch send_strategy = link_->send_strategy();
    if (send_strategy)
      send_strategy->terminate_send();
  }
}

void
OpenDDS::DCPS::TcpConnection::handle_stop_reconnecting()
{
  this->reconnect_state_ = LOST_STATE;
  notify_connection_lost();
  if (this->tcp_config_->conn_retry_attempts_ > 0) {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) we tried and failed to re-establish connection on transport: %C to %C:%d.\n",
                  this->config_name().c_str(),
                  this->remote_address_.get_host_addr(),
                  this->remote_address_.get_port_number()));
  }
  else {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) we did not try to re-establish connection on transport: %C to %C:%d.\n",
                this->config_name().c_str(),
                this->remote_address_.get_host_addr(),
                this->remote_address_.get_port_number()));
  }
}

/// A timer is scheduled on acceptor side to check if a new connection
/// is accepted after the connection is lost.
int
OpenDDS::DCPS::TcpConnection::handle_timeout(const ACE_Time_Value &,
                                             const void *)
{
  DBG_ENTRY_LVL("TcpConnection","handle_timeout",6);
  ACE_DEBUG((LM_DEBUG, "(%P|%t) TcpConnection::handle_timeout, this->reconnect_state_=%C\n",reconnect_state_string() ));
  GuardType guard(this->reconnect_lock_);

  switch (this->reconnect_state_) {
  case PASSIVE_WAITING_STATE: {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) TcpConnection::handle_timeout, we tried and failed to re-establish connection on transport: %C to %C:%d.\n",
               this->config_name().c_str(),
               this->remote_address_.get_host_addr(),
               this->remote_address_.get_port_number()));

    this->reconnect_state_ = PASSIVE_TIMEOUT_CALLED_STATE;
    // We stay in PASSIVE_TIMEOUT_CALLED_STATE indicates there is no new connection.
    // Now we need declare the connection is lost.
    this->notify_connection_lost();

    this->reconnect_state_ = LOST_STATE;

    this->tear_link();

  }
  break;

  case RECONNECTED_STATE:
    // reconnected successfully.
    ACE_DEBUG((LM_DEBUG, "(%P|%t) TcpConnection::handle_timeout, re-established connection on transport: %C to %C:%d.\n",
               this->config_name().c_str(),
               this->remote_address_.get_host_addr(),
               this->remote_address_.get_port_number()));
    break;

  case INIT_STATE: {
    // couldn't initialize connection successfully.
    ACE_DEBUG((LM_DEBUG, "(%P|%t) TcpConnection::handle_timeout, failed connection initialization due to timeout.: %C to %C:%d.\n",
               this->config_name().c_str(),
               this->remote_address_.get_host_addr(),
               this->remote_address_.get_port_number()));

    // build key and remove from service
    TcpTransport& transport = static_cast<TcpTransport&>(link_->impl());

    const bool is_loop(local_address_ == remote_address_);
    const PriorityKey key(transport_priority_, remote_address_,
                          is_loop, true /* active */);

    transport.async_connect_failed(key);
    }

    break;
  case ACTIVE_RECONNECTING_STATE: {
    // we get the timeout before the network stack reports the destination is unreachable
    // cancel the async connect operation and retry it.
    TcpTransport& transport = static_cast<TcpTransport&>(link_->impl());
    transport.connector_.cancel(this);
    this->active_reconnect_i();
    break;
  }
  case ACTIVE_WAITING_STATE:
    this->active_reconnect_i();
    break;
  case LOST_STATE:
    break;
  default :
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: TcpConnection::handle_timeout, ")
               ACE_TEXT(" unknown state or it should not be in state=%d \n"),
               reconnect_state_));
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
  GuardType guard(reconnect_lock_);

  if (shutdown_) {
    return;
  }

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
    // we just let the timer expires by itself. When the timer
    // expires, it already transitions to the RECONNECTED_STATE,
    // and do nothing in handle_timeout(). We don't need to delete
    // the timer explicitly.
    notify_reconnect = true;
  }
  break;

  default :
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: TcpConnection::transfer, ")
               ACE_TEXT(" unknown state or it should not be in state=%i \n"),
               reconnect_state_));
    break;
  }

  // Verify if this acceptor side.
  if (this->is_connector_ || connection->is_connector_) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: TcpConnection::transfer, ")
               ACE_TEXT(" should NOT be called by the connector side \n")));
  }

  connection->remote_address_ = this->remote_address_;
  connection->local_address_ = this->local_address_;
  connection->tcp_config_ = this->tcp_config_;
  connection->link_ = this->link_;
  connection->impl_ = this->impl_;

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

    }
  }

  if (notify_lost) {
    this->disconnect();
    this->notify_connection_lost();
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

  TcpSendStrategy_rch send_strategy = this->send_strategy();
  if (do_suspend && send_strategy)
    send_strategy->suspend_send();
}

/// This is called by TcpReceiveStrategy when a disconnect
/// is detected.  It simply suspends any sends and lets
/// the handle_close() handle the reconnect logic.
void
OpenDDS::DCPS::TcpConnection::relink_from_recv(bool do_suspend)
{
  DBG_ENTRY_LVL("TcpConnection","relink_from_recv",6);
  TcpSendStrategy_rch send_strategy = this->send_strategy();
  if (do_suspend && send_strategy)
    send_strategy->suspend_send();
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
  DBG_ENTRY_LVL("TcpConnection", "shutdown", 6);

  GuardType guard(reconnect_lock_);
  shutdown_ = true;

  ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_NULL_SYNCH>::shutdown();
}

ACE_Event_Handler::Reference_Count
OpenDDS::DCPS::TcpConnection::add_reference()
{
  RcObject::_add_ref();
  return 1;
}

ACE_Event_Handler::Reference_Count
OpenDDS::DCPS::TcpConnection::remove_reference()
{
  RcObject::_remove_ref();
  return 1;
}

const char* OpenDDS::DCPS::TcpConnection::reconnect_state_string() const
{
  switch (reconnect_state_) {
  case INIT_STATE:
    return "INIT_STATE";
  case LOST_STATE:
    return "LOST_STATE";
  case RECONNECTED_STATE:
    return "RECONENCTED_STATE";
  case ACTIVE_RECONNECTING_STATE:
    return "ACTIVE_RECONNECTING_STATE";
  case ACTIVE_WAITING_STATE:
    return "ACTIVE_WAITING_STATE";
  case PASSIVE_WAITING_STATE:
    return "PASSIVE_WAITING_STATE";
  case PASSIVE_TIMEOUT_CALLED_STATE:
    return "PASSIVE_TIMEOUT_CALLED_STATE";
  default:
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: TcpConnection::reconnect_state_string: ")
      ACE_TEXT("%d is either invalid or not recognized.\n"),
      reconnect_state_));
    return "Invalid reconnect state";
  }
}


int
OpenDDS::DCPS::TcpConnection::active_reconnect_open()
{
  DBG_ENTRY_LVL("TcpConnection","active_reconnect_open",6);

  ACE_DEBUG((LM_DEBUG, "(%P|%t) re-established connection on transport: %C to %C:%d.\n",
            this->config_name().c_str(),
            this->remote_address_.get_host_addr(),
            this->remote_address_.get_port_number()));

  GuardType guard(reconnect_lock_);

  if (shutdown_) {
    return 0;
  }

  if (on_active_connection_established() == -1) {
    return -1;
  }

  int result = reactor()->register_handler(this, READ_MASK);
  if (result == -1) {
      ACE_ERROR_RETURN((LM_ERROR,
                        "(%P|%t) ERROR: OpenDDS::DCPS::TcpConnection::active_reconnect_open() can't register "
                        "with reactor %X %p\n", this, ACE_TEXT("register_handler")),
                        -1);
  }

  this->reconnect_state_ = RECONNECTED_STATE;
  this->link_->notify(DataLink::RECONNECTED);
  this->send_strategy()->resume_send();
  this->conn_retry_counter_ = 0;

  return 0;
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
