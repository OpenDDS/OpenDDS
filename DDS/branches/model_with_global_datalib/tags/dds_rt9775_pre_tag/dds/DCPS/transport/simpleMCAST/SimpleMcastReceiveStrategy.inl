// -*- C++ -*-
//
// $Id$
#include  "SimpleMcastTransport.h"
#include  "SimpleMcastSocket.h"
#include  "dds/DCPS/transport/framework/TransportReactorTask.h"

#include  "dds/DCPS/transport/framework/EntryExit.h"


ACE_INLINE
TAO::DCPS::SimpleMcastReceiveStrategy::SimpleMcastReceiveStrategy
                                        (SimpleMcastTransport*   transport,
                                         SimpleMcastSocket*      socket,
                                         TransportReactorTask* task)
{
  DBG_ENTRY_LVL("SimpleMcastReceiveStrategy","SimpleMcastReceiveStrategy",5);

  // Keep copies of the references for ourselves
  transport->_add_ref();
  this->transport_ = transport;

  socket->_add_ref();
  this->socket_ = socket;

  task->_add_ref();
  this->task_ = task;
}

