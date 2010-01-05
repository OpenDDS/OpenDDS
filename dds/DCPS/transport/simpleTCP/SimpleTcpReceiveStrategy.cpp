/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "SimpleTcp_pch.h"
#include "SimpleTcpReceiveStrategy.h"
#include "SimpleTcpSendStrategy.h"
#include "SimpleTcpTransport.h"
#include "SimpleTcpDataLink.h"
#include "SimpleTcpConnection.h"

#include <sstream>

#if !defined (__ACE_INLINE__)
#include "SimpleTcpReceiveStrategy.inl"
#endif /* __ACE_INLINE__ */

OpenDDS::DCPS::SimpleTcpReceiveStrategy::SimpleTcpReceiveStrategy
(SimpleTcpDataLink*    link,
 SimpleTcpConnection*  connection,
 TransportReactorTask* task)
{
  DBG_ENTRY_LVL("SimpleTcpReceiveStrategy","SimpleTcpReceiveStrategy",6);

  // Keep a "copy" of the reference to the DataLink for ourselves.
  link->_add_ref();
  this->link_ = link;

  // Keep a "copy" of the reference to the TransportReactorTask for ourselves.
  task->_add_ref();
  this->reactor_task_ = task;

  // Keep a "copy" of the reference to the SimpleTcpConnection for ourselves.
  connection->_add_ref();
  this->connection_ = connection;
}

OpenDDS::DCPS::SimpleTcpReceiveStrategy::~SimpleTcpReceiveStrategy()
{
  DBG_ENTRY_LVL("SimpleTcpReceiveStrategy","~SimpleTcpReceiveStrategy",6);
}

ssize_t
OpenDDS::DCPS::SimpleTcpReceiveStrategy::receive_bytes
(iovec iov[],
 int   n,
 ACE_INET_Addr& remote_address)
{
  DBG_ENTRY_LVL("SimpleTcpReceiveStrategy","receive_bytes",6);

  // We don't do anything to the remote_address for the SimpleTcp case.
  ACE_UNUSED_ARG(remote_address);

  SimpleTcpConnection_rch connection = this->connection_;

  if (connection.is_nil()) {
    return 0;
  }

  return connection->peer().recvv(iov, n);
}

void
OpenDDS::DCPS::SimpleTcpReceiveStrategy::deliver_sample
(ReceivedDataSample&  sample,
 const ACE_INET_Addr& remote_address)
{
  DBG_ENTRY_LVL("SimpleTcpReceiveStrategy","deliver_sample",6);

  // We don't do anything to the remote_address for the SimpleTcp case.
  ACE_UNUSED_ARG(remote_address);

  if (sample.header_.message_id_ == GRACEFUL_DISCONNECT) {
    VDBG((LM_DEBUG, "(%P|%t) DBG:  received GRACEFUL_DISCONNECT \n"));
    this->gracefully_disconnected_ = true;

  } else if (sample.header_.message_id_ == FULLY_ASSOCIATED) {
    VDBG((LM_DEBUG, "(%P|%t) DBG:  received FULLY_ASSOCIATED \n"));

    SimpleTcpTransport_rch transport = this->link_->get_transport_impl();
    transport->demarshal_acks(sample.sample_,
                              sample.header_.byte_order_ != TAO_ENCAP_BYTE_ORDER);

  } else if (sample.header_.message_id_ == SAMPLE_ACK) {
    VDBG((LM_DEBUG, "(%P|%t) DBG:  received SAMPLE_ACK \n"));

    this->link_->ack_received(sample);

  } else
    this->link_->data_received(sample);
}

int
OpenDDS::DCPS::SimpleTcpReceiveStrategy::start_i()
{
  DBG_ENTRY_LVL("SimpleTcpReceiveStrategy","start_i",6);

  // Tell the SimpleTcpConnection that we are the object that it should
  // call when it receives a handle_input() "event", and we will carry
  // it out.  The SimpleTcpConnection object will make a "copy" of the
  // reference (to this object) that we pass-in here.
  this->connection_->set_receive_strategy(this);

  // Give the reactor its own "copy" of the reference to the connection object.
  this->connection_->_add_ref();

  if (DCPS_debug_level > 9) {
    std::stringstream buffer;
    buffer << *this->link_.in();
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) SimpleTcpReceiveStrategy::start_i() - ")
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
    this->connection_->_remove_ref();
    ACE_ERROR_RETURN((LM_ERROR,
                      "(%P|%t) ERROR: SimpleTcpReceiveStrategy::start_i SimpleTcpConnection can't register with "
                      "reactor %X %p\n", this->connection_.in(), ACE_TEXT("register_handler")),
                     -1);
  }

  return 0;
}

// This is called by the datalink object to associate with the "new" connection object.
// The "old" connection object is unregistered with the reactor and the "new" connection
// object is registered for receiving.
int
OpenDDS::DCPS::SimpleTcpReceiveStrategy::reset(SimpleTcpConnection* connection)
{
  DBG_ENTRY_LVL("SimpleTcpReceiveStrategy","reset",6);

  // Sanity check - this connection is passed in from the constructor and
  // it should not be nil.
  if (this->connection_.is_nil()) {
    ACE_ERROR_RETURN((LM_ERROR,
                      "(%P|%t) ERROR: SimpleTcpReceiveStrategy::reset  previous connection "
                      "should not be nil.\n"),
                     -1);
  }

  if (this->connection_.in() == connection) {
    ACE_ERROR_RETURN((LM_ERROR,
                      "(%P|%t) ERROR: SimpleTcpReceiveStrategy::reset should not be called"
                      " to replace the same connection.\n"),
                     -1);
  }

  // Unregister the old handle
  this->reactor_task_->get_reactor()->remove_handler
  (this->connection_.in(),
   ACE_Event_Handler::READ_MASK |
   ACE_Event_Handler::DONT_CALL);

  // Take back the "copy" we made (see start_i() implementation).
  this->connection_->_remove_ref();

  // This will cause the connection_ object to drop its reference to this
  // TransportReceiveStrategy object.
  this->connection_->remove_receive_strategy();

  // Replace with a new connection.
  connection->_add_ref();
  this->connection_ = connection;

  // Tell the SimpleTcpConnection that we are the object that it should
  // call when it receives a handle_input() "event", and we will carry
  // it out.  The SimpleTcpConnection object will make a "copy" of the
  // reference (to this object) that we pass-in here.
  this->connection_->set_receive_strategy(this);

  // Give the reactor its own "copy" of the reference to the connection object.
  this->connection_->_add_ref();

  if (this->reactor_task_->get_reactor()->register_handler
      (this->connection_.in(),
       ACE_Event_Handler::READ_MASK) == -1) {
    // Take back the "copy" we made.
    this->connection_->_remove_ref();
    ACE_ERROR_RETURN((LM_ERROR,
                      "(%P|%t) ERROR: SimpleTcpReceiveStrategy::reset SimpleTcpConnection can't register with "
                      "reactor\n"),
                     -1);
  }

  return 0;
}

void
OpenDDS::DCPS::SimpleTcpReceiveStrategy::stop_i()
{
  DBG_ENTRY_LVL("SimpleTcpReceiveStrategy","stop_i",6);

  this->reactor_task_->get_reactor()->remove_handler
  (this->connection_.in(),
   ACE_Event_Handler::READ_MASK |
   ACE_Event_Handler::DONT_CALL);

  // Take back the "copy" we made (see start_i() implementation).
  this->connection_->_remove_ref();

  // This will cause the connection_ object to drop its reference to this
  // TransportReceiveStrategy object.
  this->connection_->remove_receive_strategy();

  this->connection_ = 0;
}

void
OpenDDS::DCPS::SimpleTcpReceiveStrategy::relink(bool do_suspend)
{
  DBG_ENTRY_LVL("SimpleTcpReceiveStrategy","relink",6);

  if (!this->connection_.is_nil())
    this->connection_->relink(do_suspend);
}
