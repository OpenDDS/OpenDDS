/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "Tcp_pch.h"
#include "TcpConnection.h"
#include "TcpSendStrategy.h"
#include "TcpTransport.h"
#include "TcpInst.h"
#include "TcpSynchResource.h"
#include "TcpDataLink.h"
#include "dds/DCPS/transport/framework/TransportReactorTask.h"
#include "dds/DCPS/transport/framework/PerConnectionSynchStrategy.h"

#ifndef DEVELOPMENT
#define DEVELOPMENT 0 // DEVELOPMENT DIAGNOSTICS ONLY
#endif

OpenDDS::DCPS::TcpSendStrategy::TcpSendStrategy(
  std::size_t id,
  const TcpDataLink_rch& link,
  const TcpInst_rch& config,
  const TcpConnection_rch& connection,
  TcpSynchResource* synch_resource,
  const TransportReactorTask_rch& task,
  CORBA::Long priority)
  : TransportSendStrategy(id, static_rchandle_cast<TransportInst>(config),
                          synch_resource, priority,
                          new PerConnectionSynchStrategy)
  , pending_output_(false)
  , connection_(connection)
  , link_(link)
  , reactor_task_(task)
{
  DBG_ENTRY_LVL("TcpSendStrategy","TcpSendStrategy",6);

  connection->set_send_strategy(this);
}

OpenDDS::DCPS::TcpSendStrategy::~TcpSendStrategy()
{
  DBG_ENTRY_LVL("TcpSendStrategy","~TcpSendStrategy",6);
}

int
OpenDDS::DCPS::TcpSendStrategy::schedule_wakeup( ACE_Reactor_Mask masks_to_be_added)
{
  if(!pending_output_) {
    if(DEVELOPMENT) {
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) TcpSendStrategy::schedule_wakeup() [%d] - ")
                 ACE_TEXT("starting data queueing.\n"),
                 id()));
    }
    pending_output_ = true;
    return reactor_task_->get_reactor()->schedule_wakeup( get_handle(), masks_to_be_added);

  } else {
    return 0;
  }
}

int
OpenDDS::DCPS::TcpSendStrategy::cancel_wakeup( ACE_Reactor_Mask masks_to_be_cleared)
{
  if(pending_output_) {
    if(DEVELOPMENT) {
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) TcpSendStrategy::cancel_wakeup() [%d] - ")
                 ACE_TEXT("stopping data queueing.\n"),
                 id()));
    }
    pending_output_ = false;
    return reactor_task_->get_reactor()->cancel_wakeup( get_handle(), masks_to_be_cleared);

  } else {
    return 0;
  }
}

int
OpenDDS::DCPS::TcpSendStrategy::reset(TcpConnection* connection)
{
  DBG_ENTRY_LVL("TcpSendStrategy","reset",6);

  // Sanity check - this connection is passed in from the constructor and
  // it should not be nil.
  if (this->connection_.is_nil()) {
    ACE_ERROR_RETURN((LM_ERROR,
                      "(%P|%t) ERROR: TcpSendStrategy::reset  previous connection "
                      "should not be nil.\n"),
                     -1);
  }

  if (this->connection_.in() == connection) {
    ACE_ERROR_RETURN((LM_ERROR,
                      "(%P|%t) ERROR: TcpSendStrategy::reset should not be called"
                      " to replace the same connection.\n"),
                     -1);
  }

  // This will cause the connection_ object to drop its reference to this
  // TransportSendStrategy object.
  this->connection_->remove_send_strategy();

  // Replace with a new connection.
  connection->_add_ref();
  this->connection_ = connection;

  // Tell the TcpConnection that we are the object that it should
  // call when it receives a handle_input() "event", and we will carry
  // it out.  The TcpConnection object will make a "copy" of the
  // reference (to this object) that we pass-in here.
  this->connection_->set_send_strategy(this);

  return 0;
}

ssize_t
OpenDDS::DCPS::TcpSendStrategy::send_bytes(const iovec iov[], int n, int& bp)
{
  DBG_ENTRY_LVL("TcpSendStrategy","send_bytes",6);

  return this->non_blocking_send(iov, n, bp);
}

ACE_HANDLE
OpenDDS::DCPS::TcpSendStrategy::get_handle()
{
  TcpConnection_rch connection = this->connection_;

  if (connection.is_nil())
    return ACE_INVALID_HANDLE;

  return connection->peer().get_handle();
}

ssize_t
OpenDDS::DCPS::TcpSendStrategy::send_bytes_i(const iovec iov[], int n)
{
  TcpConnection_rch connection = this->connection_;

  if (connection.is_nil())
    return -1;

  return connection->peer().sendv(iov, n);
}

void
OpenDDS::DCPS::TcpSendStrategy::relink(bool do_suspend)
{
  DBG_ENTRY_LVL("TcpSendStrategy","relink",6);

  if (!this->connection_.is_nil())
    this->connection_->relink(do_suspend);
}

void
OpenDDS::DCPS::TcpSendStrategy::stop_i()
{
  DBG_ENTRY_LVL("TcpSendStrategy","stop_i",6);

  // This will cause the connection_ object to drop its reference to this
  // TransportSendStrategy object.
  this->connection_->remove_send_strategy();

  // Take back the "copy" of connection object given. (see constructor).
  this->connection_ = 0;
}
