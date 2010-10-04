// -*- C++ -*-
//
// $Id$

#include "DummyTcp_pch.h"
#include "DummyTcpReceiveStrategy.h"
#include "DummyTcpSendStrategy.h"
#include "DummyTcpTransport.h"
#include "DummyTcpDataLink.h"
#include "PerformanceTest.h"


#if !defined (__ACE_INLINE__)
#include "DummyTcpReceiveStrategy.inl"
#endif /* __ACE_INLINE__ */

OpenDDS::DCPS::DummyTcpReceiveStrategy::DummyTcpReceiveStrategy
                                        (DummyTcpDataLink*    link,
                                         DummyTcpConnection*  connection,
                                         TransportReactorTask* task)
{
  DBG_ENTRY_LVL("DummyTcpReceiveStrategy","DummyTcpReceiveStrategy",5);

  // Keep a "copy" of the reference to the DataLink for ourselves.
  link->_add_ref();
  this->link_ = link;

  // Keep a "copy" of the reference to the TransportReactorTask for ourselves.
  task->_add_ref();
  this->reactor_task_ = task;

  // Keep a "copy" of the reference to the DummyTcpConnection for ourselves.
  connection->_add_ref();
  this->connection_ = connection;
}


OpenDDS::DCPS::DummyTcpReceiveStrategy::~DummyTcpReceiveStrategy()
{
  DBG_ENTRY_LVL("DummyTcpReceiveStrategy","~DummyTcpReceiveStrategy",5);
}


ssize_t
OpenDDS::DCPS::DummyTcpReceiveStrategy::receive_bytes
                                             (iovec iov[],
                                              int   n,
                                              ACE_INET_Addr& remote_address)
{
  DBG_ENTRY_LVL("DummyTcpReceiveStrategy","receive_bytes",5);

  // We don't do anything to the remote_address for the DummyTcp case.
  ACE_UNUSED_ARG(remote_address);

  // begin the  performance test on the subscriber side
  PerformanceTest::start_test ("Subscriber Side Transport Performance Test",
                               "OpenDDS::DCPS::DummyTcpReceiveStrategy::receive_bytes");

  if (this->connection_.is_nil())
    {
      return 0;
    }

  return this->connection_->peer().recvv(iov, n);
}


void
OpenDDS::DCPS::DummyTcpReceiveStrategy::deliver_sample
                                 (ReceivedDataSample&  sample,
                                  const ACE_INET_Addr& remote_address)
{
  DBG_ENTRY_LVL("DummyTcpReceiveStrategy","deliver_sample",5);

  // We don't do anything to the remote_address for the DummyTcp case.
  ACE_UNUSED_ARG(remote_address);
  if (sample.header_.message_id_ == GRACEFUL_DISCONNECT)
    {
      VDBG((LM_DEBUG, "(%P|%t) DBG:  received GRACEFUL_DISCONNECT \n"));
      this->gracefully_disconnected_ = true;
    }
  else if (sample.header_.message_id_ == FULLY_ASSOCIATED)
    {
      VDBG((LM_DEBUG, "(%P|%t) DBG:  received FULLY_ASSOCIATED \n"));

      DummyTcpTransport_rch transport = this->link_->get_transport_impl();
      transport->demarshal_acks (sample.sample_,
                                 sample.header_.byte_order_ != TAO_ENCAP_BYTE_ORDER);
    }
  else
    this->link_->data_received(sample);
}


int
OpenDDS::DCPS::DummyTcpReceiveStrategy::start_i()
{
  DBG_ENTRY_LVL("DummyTcpReceiveStrategy","start_i",5);

  // Tell the DummyTcpConnection that we are the object that it should
  // call when it receives a handle_input() "event", and we will carry
  // it out.  The DummyTcpConnection object will make a "copy" of the
  // reference (to this object) that we pass-in here.
  this->connection_->set_receive_strategy(this);

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


// This is called by the datalink object to associate with the "new" connection object.
// The "old" connection object is unregistered with the reactor and the "new" connection
// object is registered for receiving.
int
OpenDDS::DCPS::DummyTcpReceiveStrategy::reset(DummyTcpConnection* connection)
{
  DBG_ENTRY_LVL("DummyTcpReceiveStrategy","reset",5);

  // Sanity check - this connection is passed in from the constructor and
  // it should not be nil.
  if (this->connection_.is_nil())
    {
      ACE_ERROR_RETURN((LM_ERROR,
                        "(%P|%t) ERROR: DummyTcpReceiveStrategy::reset  previous connection "
                        "should not be nil.\n"),
                       -1);
    }

  if (this->connection_.in () == connection)
    {
      ACE_ERROR_RETURN((LM_ERROR,
                        "(%P|%t) ERROR: DummyTcpReceiveStrategy::reset should not be called"
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
  connection->_add_ref ();
  this->connection_ = connection;

  // Tell the DummyTcpConnection that we are the object that it should
  // call when it receives a handle_input() "event", and we will carry
  // it out.  The DummyTcpConnection object will make a "copy" of the
  // reference (to this object) that we pass-in here.
  this->connection_->set_receive_strategy(this);

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


void
OpenDDS::DCPS::DummyTcpReceiveStrategy::stop_i()
{
  DBG_ENTRY_LVL("DummyTcpReceiveStrategy","stop_i",5);

  this->reactor_task_->get_reactor()->remove_handler
                                             (this->connection_.in(),
                                              ACE_Event_Handler::READ_MASK |
                                              ACE_Event_Handler::DONT_CALL);

  // Take back the "copy" we made (see start_i() implementation).
  this->connection_->_remove_ref();

  // This will cause the connection_ object to drop its reference to this
  // TransportReceiveStrategy object.
  this->connection_->remove_receive_strategy();
}


void
OpenDDS::DCPS::DummyTcpReceiveStrategy::relink (bool do_suspend)
{
  DBG_ENTRY_LVL("DummyTcpReceiveStrategy","relink",5);
  this->connection_->relink (do_suspend);
}
