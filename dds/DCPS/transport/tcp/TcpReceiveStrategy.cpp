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
#include <dds/DCPS/LogAddr.h>

#include <sstream>

#if !defined (__ACE_INLINE__)
#include "TcpReceiveStrategy.inl"
#endif /* __ACE_INLINE__ */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

OpenDDS::DCPS::TcpReceiveStrategy::TcpReceiveStrategy(
  TcpDataLink& link,
  const ReactorTask_rch& task)
  : link_(link)
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
  ACE_HANDLE /*fd*/,
  bool& /*stop*/)
{
  DBG_ENTRY_LVL("TcpReceiveStrategy", "receive_bytes", 6);

  // We don't do anything to the remote_address for the Tcp case.
  TcpConnection_rch connection = link_.get_connection();
  if (!connection) {
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
    VDBG((LM_DEBUG, "(%P|%t) DBG:  received GRACEFUL_DISCONNECT\n"));
    this->gracefully_disconnected_ = true;
  }
  else if (sample.header_.message_id_ == REQUEST_ACK) {
    VDBG((LM_DEBUG, "(%P|%t) DBG:  received REQUEST_ACK\n"));
    link_.request_ack_received(sample);
  }
  else if (sample.header_.message_id_ == SAMPLE_ACK) {
    VDBG((LM_DEBUG, "(%P|%t) DBG:  received SAMPLE_ACK\n"));
    link_.ack_received(sample);
  }
  else {
    link_.data_received(sample);
  }
}

int
OpenDDS::DCPS::TcpReceiveStrategy::start_i()
{
  DBG_ENTRY_LVL("TcpReceiveStrategy","start_i",6);

  TcpConnection_rch connection = link_.get_connection();

  if (DCPS_debug_level > 9) {
    std::stringstream buffer;
    buffer << link_;
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) TcpReceiveStrategy::start_i() - ")
               ACE_TEXT("link:\n%C connected to %C ")
               ACE_TEXT("registering with reactor to receive.\n"),
               buffer.str().c_str(),
               LogAddr(connection->get_remote_address()).c_str()));
  }

  if (this->reactor_task_->get_reactor()->register_handler
      (connection.in(),
       ACE_Event_Handler::READ_MASK) == -1) {
    // Take back the "copy" we made.
    ACE_ERROR_RETURN((LM_ERROR,
                      "(%P|%t) ERROR: TcpReceiveStrategy::start_i TcpConnection can't register with "
                      "reactor %@ %p\n", connection.in(), ACE_TEXT("register_handler")),
                     -1);
  }

  return 0;
}

// This is called by the datalink object to associate with the "new" connection object.
// The "old" connection object is unregistered with the reactor and the "new" connection
// object is registered for receiving.
int
OpenDDS::DCPS::TcpReceiveStrategy::reset(TcpConnection* old_connection, TcpConnection* new_connection)
{
  DBG_ENTRY_LVL("TcpReceiveStrategy","reset",6);
  // Unregister the old handle
  if (old_connection) {
    this->reactor_task_->get_reactor()->remove_handler
    (old_connection,
     ACE_Event_Handler::READ_MASK |
     ACE_Event_Handler::DONT_CALL);
   }

   link_.drop_pending_request_acks();

  // Give the reactor its own "copy" of the reference to the connection object.

  if (this->reactor_task_->get_reactor()->register_handler
      (new_connection,
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

  link_.drop_pending_request_acks();
}

void
OpenDDS::DCPS::TcpReceiveStrategy::relink(bool do_suspend)
{
  DBG_ENTRY_LVL("TcpReceiveStrategy","relink",6);
  TcpConnection_rch connection = link_.get_connection();
  if (connection)
    connection->relink_from_recv(do_suspend);
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
