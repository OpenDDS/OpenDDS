/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "SimpleUnreliableDgramTransport.h"
#include "SimpleUnreliableDgramSocket.h"
#include "dds/DCPS/transport/framework/TransportReactorTask.h"

#include "dds/DCPS/transport/framework/EntryExit.h"

ACE_INLINE
OpenDDS::DCPS::SimpleUnreliableDgramReceiveStrategy::SimpleUnreliableDgramReceiveStrategy
(SimpleUnreliableDgramTransport*   transport,
 SimpleUnreliableDgramSocket* socket,
 TransportReactorTask* task)
{
  DBG_ENTRY_LVL("SimpleUnreliableDgramReceiveStrategy","SimpleUnreliableDgramReceiveStrategy",6);

  // Keep copies of the references for ourselves
  transport->_add_ref();
  this->transport_ = transport;

  socket->_add_ref();
  this->socket_ = socket;

  task->_add_ref();
  this->task_ = task;
}
