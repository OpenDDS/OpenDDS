// -*- C++ -*-
//
// $Id$

#include  "SimpleTcpConnection.h"
#include  "SimpleTcpDataLink.h"
#include  "dds/DCPS/transport/framework/TransportReactorTask.h"
#include  "dds/DCPS/transport/framework/EntryExit.h"

ACE_INLINE
TAO::DCPS::SimpleTcpReceiveStrategy::SimpleTcpReceiveStrategy
                                        (SimpleTcpDataLink*    link,
                                         SimpleTcpConnection*  connection,
                                         TransportReactorTask* task)
{
  DBG_ENTRY("SimpleTcpReceiveStrategy","SimpleTcpReceiveStrategy");

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


ACE_INLINE ACE_Reactor*
TAO::DCPS::SimpleTcpReceiveStrategy::get_reactor()
{
  DBG_ENTRY("SimpleTcpReceiveStrategy","get_reactor");
  return this->reactor_task_->get_reactor ();
}

