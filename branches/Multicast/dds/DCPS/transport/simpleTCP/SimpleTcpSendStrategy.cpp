/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "SimpleTcp_pch.h"
#include "SimpleTcpConnection.h"
#include "SimpleTcpSendStrategy.h"
#include "SimpleTcpTransport.h"
#include "SimpleTcpConfiguration.h"
#include "SimpleTcpSynchResource.h"
#include "SimpleTcpDataLink.h"
#include "dds/DCPS/transport/framework/TransportReactorTask.h"

OpenDDS::DCPS::SimpleTcpSendStrategy::SimpleTcpSendStrategy(
  SimpleTcpDataLink*      link,
  SimpleTcpConfiguration* config,
  SimpleTcpConnection*    connection,
  SimpleTcpSynchResource* synch_resource,
  TransportReactorTask*   task,
  CORBA::Long             priority)
  : TransportSendStrategy(config, synch_resource, priority)
{
  DBG_ENTRY_LVL("SimpleTcpSendStrategy","SimpleTcpSendStrategy",6);

  // Keep a "copy" of the connection reference for ourselves
  connection->_add_ref();
  this->connection_ = connection;

  // Give a "copy" of this send strategy to the connection object.
  connection->set_send_strategy(this);

  // Keep a "copy" of the SimpleTcpDataLink reference for ourselves
  link->_add_ref();
  this->link_ = link;

  // Keep a "copy" of the reference to the TransportReactorTask for ourselves.
  task->_add_ref();
  this->reactor_task_ = task;

}

OpenDDS::DCPS::SimpleTcpSendStrategy::~SimpleTcpSendStrategy()
{
  DBG_ENTRY_LVL("SimpleTcpSendStrategy","~SimpleTcpSendStrategy",6);
}

int
OpenDDS::DCPS::SimpleTcpSendStrategy::reset(SimpleTcpConnection* connection)
{
  DBG_ENTRY_LVL("SimpleTcpSendStrategy","reset",6);

  // Sanity check - this connection is passed in from the constructor and
  // it should not be nil.
  if (this->connection_.is_nil()) {
    ACE_ERROR_RETURN((LM_ERROR,
                      "(%P|%t) ERROR: SimpleTcpSendStrategy::reset  previous connection "
                      "should not be nil.\n"),
                     -1);
  }

  if (this->connection_.in() == connection) {
    ACE_ERROR_RETURN((LM_ERROR,
                      "(%P|%t) ERROR: SimpleTcpSendStrategy::reset should not be called"
                      " to replace the same connection.\n"),
                     -1);
  }

  // This will cause the connection_ object to drop its reference to this
  // TransportSendStrategy object.
  this->connection_->remove_send_strategy();

  // Replace with a new connection.
  connection->_add_ref();
  this->connection_ = connection;

  // Tell the SimpleTcpConnection that we are the object that it should
  // call when it receives a handle_input() "event", and we will carry
  // it out.  The SimpleTcpConnection object will make a "copy" of the
  // reference (to this object) that we pass-in here.
  this->connection_->set_send_strategy(this);

  return 0;
}

ssize_t
OpenDDS::DCPS::SimpleTcpSendStrategy::send_bytes(const iovec iov[], int n, int& bp)
{
  DBG_ENTRY_LVL("SimpleTcpSendStrategy","send_bytes",6);

  return this->non_blocking_send(iov, n, bp);
}

ACE_HANDLE
OpenDDS::DCPS::SimpleTcpSendStrategy::get_handle()
{
  SimpleTcpConnection_rch connection = this->connection_;

  if (connection.is_nil())
    return ACE_INVALID_HANDLE;

  return connection->peer().get_handle();
}

ssize_t
OpenDDS::DCPS::SimpleTcpSendStrategy::send_bytes_i(const iovec iov[], int n)
{
  SimpleTcpConnection_rch connection = this->connection_;

  if (connection.is_nil())
    return -1;

  return connection->peer().sendv(iov, n);
}

void
OpenDDS::DCPS::SimpleTcpSendStrategy::relink(bool do_suspend)
{
  DBG_ENTRY_LVL("SimpleTcpSendStrategy","relink",6);

  if (!this->connection_.is_nil())
    this->connection_->relink(do_suspend);
}

void
OpenDDS::DCPS::SimpleTcpSendStrategy::stop_i()
{
  DBG_ENTRY_LVL("SimpleTcpSendStrategy","stop_i",6);

  // This will cause the connection_ object to drop its reference to this
  // TransportSendStrategy object.
  this->connection_->remove_send_strategy();

  // Take back the "copy" of connection object given. (see constructor).
  this->connection_ = 0;
}
