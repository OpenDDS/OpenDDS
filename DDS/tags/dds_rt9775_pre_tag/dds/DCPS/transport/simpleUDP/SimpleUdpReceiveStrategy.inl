// -*- C++ -*-
//
// $Id$
#include  "SimpleUdpTransport.h"
#include  "SimpleUdpSocket.h"
#include  "dds/DCPS/transport/framework/TransportReactorTask.h"

#include  "dds/DCPS/transport/framework/EntryExit.h"


ACE_INLINE
TAO::DCPS::SimpleUdpReceiveStrategy::SimpleUdpReceiveStrategy
                                        (SimpleUdpTransport*   transport,
                                         SimpleUdpSocket*      socket,
                                         TransportReactorTask* task)
{
  DBG_ENTRY_LVL("SimpleUdpReceiveStrategy","SimpleUdpReceiveStrategy",5);

  // Keep copies of the references for ourselves
  transport->_add_ref();
  this->transport_ = transport;

  socket->_add_ref();
  this->socket_ = socket;

  task->_add_ref();
  this->task_ = task;
}

