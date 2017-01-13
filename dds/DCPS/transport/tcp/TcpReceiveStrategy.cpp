/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "Tcp_pch.h"
#include "TcpReceiveStrategy.h"
#include "TcpSendStrategy.h"
#include "TcpTransport.h"
#include "TcpDataLink.h"
#include "TcpConnection.h"

#include <sstream>

#if !defined (__ACE_INLINE__)
#include "TcpReceiveStrategy.inl"
#endif /* __ACE_INLINE__ */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

OpenDDS::DCPS::TcpReceiveStrategy::TcpReceiveStrategy(
  const TcpDataLink_rch& link,
  const TcpConnection_rch& connection,
  const TransportReactorTask_rch& task)
  : link_(link)
  , connection_(connection)
  , reactor_task_(task)
{
  DBG_ENTRY_LVL("TcpReceiveStrategy","TcpReceiveStrategy",6);
}

OpenDDS::DCPS::TcpReceiveStrategy::~TcpReceiveStrategy()
{
  DBG_ENTRY_LVL("TcpReceiveStrategy","~TcpReceiveStrategy",6);
}

ssize_t
OpenDDS::DCPS::TcpReceiveStrategy::receive_bytes(
  iovec iov[],
  int   n,
  ACE_INET_Addr& /*remote_address*/,
  ACE_HANDLE /*fd*/)
{
  DBG_ENTRY_LVL("TcpReceiveStrategy", "receive_bytes", 6);

  // We don't do anything to the remote_address for the Tcp case.

  TcpConnection_rch connection = this->connection_;

  if (connection.is_nil()) {
    return 0;
  }

  return connection->peer().recvv(iov, n);
}

void
OpenDDS::DCPS::TcpReceiveStrategy::deliver_sample
  (ReceivedDataSample& sample, const ACE_INET_Addr&)
{
  DBG_ENTRY_LVL("TcpReceiveStrategy","deliver_sample",6);
  if (sample.header_.message_id_ == GRACEFUL_DISCONNECT) {
    VDBG((LM_DEBUG, "(%P|%t) DBG:  received GRACEFUL_DISCONNECT \n"));
    this->gracefully_disconnected_ = true;

  } else {
    this->link_->data_received(sample);
  }
}

int
OpenDDS::DCPS::TcpReceiveStrategy::start_i()
{
  DBG_ENTRY_LVL("TcpReceiveStrategy","start_i",6);

  // Tell the TcpConnection that we are the object that it should
  // call when it receives a handle_input() "event", and we will carry
  // it out.  The TcpConnection object will make a "copy" of the
  // reference (to this object) that we pass-in here.
  this->connection_->set_receive_strategy(rchandle_from(this));

  if (connection_->is_connector()) {
    // Give the reactor its own reference to the connection object.
    // If ACE_Acceptor was used, the reactor already has this reference
  }

  if (DCPS_debug_level > 9) {
    std::stringstream buffer;
    buffer << *this->link_.in();
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) TcpReceiveStrategy::start_i() - ")
               ACE_TEXT("link:\n%C connected to %C:%d ")
               ACE_TEXT("registering with reactor to receive.\n"),
               buffer.str().c_str(),
               this->connection_->get_remote_address().get_host_name(),
               this->connection_->get_remote_address().get_port_number()));
  }

  if (this->reactor_task_->get_reactor()->register_handler
      (this->connection_.in(),
       ACE_Event_Handler::READ_MASK) == -1) {
    // Take back the "copy" we made.
    ACE_ERROR_RETURN((LM_ERROR,
                      "(%P|%t) ERROR: TcpReceiveStrategy::start_i TcpConnection can't register with "
                      "reactor %@ %p\n", this->connection_.in(), ACE_TEXT("register_handler")),
                     -1);
  }

  return 0;
}

// This is called by the datalink object to associate with the "new" connection object.
// The "old" connection object is unregistered with the reactor and the "new" connection
// object is registered for receiving.
int
OpenDDS::DCPS::TcpReceiveStrategy::reset(const TcpConnection_rch& connection)
{
  DBG_ENTRY_LVL("TcpReceiveStrategy","reset",6);

  // Sanity check - this connection is passed in from the constructor and
  // it should not be nil.
  if (this->connection_.is_nil()) {
    ACE_ERROR_RETURN((LM_ERROR,
                      "(%P|%t) ERROR: TcpReceiveStrategy::reset  previous connection "
                      "should not be nil.\n"),
                     -1);
  }

  if (this->connection_ == connection) {
    ACE_ERROR_RETURN((LM_ERROR,
                      "(%P|%t) ERROR: TcpReceiveStrategy::reset should not be called"
                      " to replace the same connection.\n"),
                     -1);
  }

  // Unregister the old handle
  this->reactor_task_->get_reactor()->remove_handler
  (this->connection_.in(),
   ACE_Event_Handler::READ_MASK |
   ACE_Event_Handler::DONT_CALL);


  // This will cause the connection_ object to drop its reference to this
  // TransportReceiveStrategy object.
  this->connection_->remove_receive_strategy();

  // Replace with a new connection.
  this->connection_ = connection;

  // Tell the TcpConnection that we are the object that it should
  // call when it receives a handle_input() "event", and we will carry
  // it out.  The TcpConnection object will make a "copy" of the
  // reference (to this object) that we pass-in here.
  this->connection_->set_receive_strategy(rchandle_from(this));

  // Give the reactor its own "copy" of the reference to the connection object.

  if (this->reactor_task_->get_reactor()->register_handler
      (this->connection_.in(),
       ACE_Event_Handler::READ_MASK) == -1) {
    // Take back the "copy" we made.
    ACE_ERROR_RETURN((LM_ERROR,
                      "(%P|%t) ERROR: TcpReceiveStrategy::reset TcpConnection can't register with "
                      "reactor\n"),
                     -1);
  }

  return 0;
}

void
OpenDDS::DCPS::TcpReceiveStrategy::stop_i()
{
  DBG_ENTRY_LVL("TcpReceiveStrategy","stop_i",6);

  this->reactor_task_->get_reactor()->remove_handler
  (this->connection_.in(),
   ACE_Event_Handler::READ_MASK |
   ACE_Event_Handler::DONT_CALL);

  // This will cause the connection_ object to drop its reference to this
  // TransportReceiveStrategy object.
  this->connection_->remove_receive_strategy();
  this->connection_.reset();
}

void
OpenDDS::DCPS::TcpReceiveStrategy::relink(bool do_suspend)
{
  DBG_ENTRY_LVL("TcpReceiveStrategy","relink",6);

  if (!this->connection_.is_nil())
    this->connection_->relink_from_recv(do_suspend);
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
