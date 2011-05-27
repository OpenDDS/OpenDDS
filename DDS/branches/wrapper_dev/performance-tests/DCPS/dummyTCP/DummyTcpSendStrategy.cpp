// -*- C++ -*-
//
// $Id$

#include "DummyTcp_pch.h"
#include "DummyTcpConnection.h"
#include "DummyTcpSendStrategy.h"
#include "DummyTcpTransport.h"
#include "DummyTcpConfiguration.h"
#include "DummyTcpSynchResource.h"
#include "DummyTcpDataLink.h"
#include "dds/DCPS/transport/framework/TransportReactorTask.h"
#include "performance-tests/DCPS/dummyTCP/PerformanceTest.h"

OpenDDS::DCPS::DummyTcpSendStrategy::DummyTcpSendStrategy
                                     (DummyTcpDataLink*      link,
                                      DummyTcpConfiguration* config,
                                      DummyTcpConnection*    connection,
                                      DummyTcpSynchResource* synch_resource,
                                      TransportReactorTask* task)
  : TransportSendStrategy(config, synch_resource)
{
  DBG_ENTRY_LVL("DummyTcpSendStrategy","DummyTcpSendStrategy",5);

  // Keep a "copy" of the connection reference for ourselves
  connection->_add_ref();
  this->connection_ = connection;

  // Give a "copy" of this send strategy to the connection object.
  connection->set_send_strategy (this);

  // Keep a "copy" of the DummyTcpDataLink reference for ourselves
  link->_add_ref();
  this->link_ = link;

  // Keep a "copy" of the reference to the TransportReactorTask for ourselves.
  task->_add_ref();
  this->reactor_task_ = task;

}


OpenDDS::DCPS::DummyTcpSendStrategy::~DummyTcpSendStrategy()
{
  DBG_ENTRY_LVL("DummyTcpSendStrategy","~DummyTcpSendStrategy",5);
}

int
OpenDDS::DCPS::DummyTcpSendStrategy::reset(DummyTcpConnection* connection)
{
  DBG_ENTRY_LVL("DummyTcpSendStrategy","reset",5);

  // Sanity check - this connection is passed in from the constructor and
  // it should not be nil.
  if (this->connection_.is_nil())
    {
      ACE_ERROR_RETURN((LM_ERROR,
                        "(%P|%t) ERROR: DummyTcpSendStrategy::reset  previous connection "
                        "should not be nil.\n"),
                       -1);
    }

  if (this->connection_.in () == connection)
    {
      ACE_ERROR_RETURN((LM_ERROR,
                        "(%P|%t) ERROR: DummyTcpSendStrategy::reset should not be called"
                        " to replace the same connection.\n"),
                       -1);
    }

  // Unregister the old handle
  this->reactor_task_->get_reactor()->remove_handler
                                             (this->connection_.in(),
                                              ACE_Event_Handler::READ_MASK |
                                              ACE_Event_Handler::DONT_CALL);

  // This will cause the connection_ object to drop its reference to this
  // TransportSendStrategy object.
  this->connection_->remove_send_strategy();

  // Take back the "copy" we made (see start_i() implementation).
  this->connection_->_remove_ref();

  // Replace with a new connection.
  connection->_add_ref ();
  this->connection_ = connection;

  // Tell the DummyTcpConnection that we are the object that it should
  // call when it receives a handle_input() "event", and we will carry
  // it out.  The DummyTcpConnection object will make a "copy" of the
  // reference (to this object) that we pass-in here.
  this->connection_->set_send_strategy(this);

  // Give the reactor its own "copy" of the reference to the connection object.
  this->connection_->_add_ref();

  if (this->reactor_task_->get_reactor()->register_handler
                                      (this->connection_.in(),
                                       ACE_Event_Handler::READ_MASK) == -1)
    {
      // Take back the "copy" we made.
      this->connection_->_remove_ref();
      ACE_ERROR_RETURN((LM_ERROR,
                        "(%P|%t) ERROR: DummyTcpConnection can't register with "
                        "reactor\n"),
                       -1);
    }

  return 0;
}


ssize_t
OpenDDS::DCPS::DummyTcpSendStrategy::send_bytes(const iovec iov[], int n, int& bp)
{
  DBG_ENTRY_LVL("DummyTcpSendStrategy","send_bytes",5);

  return this->non_blocking_send (iov, n, bp);
}

ACE_HANDLE
OpenDDS::DCPS::DummyTcpSendStrategy::get_handle ()
{
  return this->connection_->peer().get_handle();
}


ssize_t
OpenDDS::DCPS::DummyTcpSendStrategy::send_bytes_i (const iovec iov[], int n)
{
  PerformanceTest::stop_test ("Publisher Side Transport Performance Test",
                              "OpenDDS::DCPS::DummyTcpSendStrategy::send_bytes_i");
  return this->connection_->peer().sendv(iov, n);
}


void
OpenDDS::DCPS::DummyTcpSendStrategy::relink (bool do_suspend)
{
  DBG_ENTRY_LVL("DummyTcpSendStrategy","relink",5);
  this->connection_->relink (do_suspend);
}

void
OpenDDS::DCPS::DummyTcpSendStrategy::stop_i()
{
  DBG_ENTRY_LVL("DummyTcpSendStrategy","stop_i",5);

  // This will cause the connection_ object to drop its reference to this
  // TransportSendStrategy object.
  this->connection_->remove_send_strategy();

  // Take back the "copy" of connection object given. (see constructor).
  this->connection_ = 0;
}

