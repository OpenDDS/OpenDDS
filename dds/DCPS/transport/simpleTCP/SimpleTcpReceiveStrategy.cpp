// -*- C++ -*-
//
// $Id$

#include  "DCPS/DdsDcps_pch.h"
#include  "SimpleTcpReceiveStrategy.h"
#include  "dds/DCPS/transport/framework/TransportReactorTask.h"


#if !defined (__ACE_INLINE__)
#include "SimpleTcpReceiveStrategy.inl"
#endif /* __ACE_INLINE__ */

TAO::DCPS::SimpleTcpReceiveStrategy::~SimpleTcpReceiveStrategy()
{
  DBG_ENTRY("SimpleTcpReceiveStrategy","~SimpleTcpReceiveStrategy");
}


ssize_t
TAO::DCPS::SimpleTcpReceiveStrategy::receive_bytes
                                             (iovec iov[],
                                              int   n,
                                              ACE_INET_Addr& remote_address)
{
  DBG_ENTRY("SimpleTcpReceiveStrategy","receive_bytes");

  // We don't do anything to the remote_address for the SimpleTcp case.
  ACE_UNUSED_ARG(remote_address);

  if (this->connection_.is_nil())
    {
      return 0;
    }

  return this->connection_->peer().recvv(iov, n);
}


void
TAO::DCPS::SimpleTcpReceiveStrategy::deliver_sample
                                 (ReceivedDataSample&  sample,
                                  const ACE_INET_Addr& remote_address)
{
  // We don't do anything to the remote_address for the SimpleTcp case.
  ACE_UNUSED_ARG(remote_address);

  this->link_->data_received(sample);
}


int
TAO::DCPS::SimpleTcpReceiveStrategy::start_i()
{
  DBG_ENTRY("SimpleTcpReceiveStrategy","start_i");

  // Tell the SimpleTcpConnection that we are the object that it should
  // call when it receives a handle_input() "event", and we will carry
  // it out.  The SimpleTcpConnection object will make a "copy" of the
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
                        "(%P|%t) ERROR: SimpleTcpConnection can't register with "
                        "reactor\n"),
                       -1);
    }

  return 0;
}


//This is called by the datalink object to associate with the "new" connection object.
//The "old" connection object is unregistered with the reactor and the "new" connection 
//object is registered for receiving.
int
TAO::DCPS::SimpleTcpReceiveStrategy::reset(SimpleTcpConnection* connection)
{
  DBG_ENTRY("SimpleTcpReceiveStrategy","reset");

  // Sanity check - this connection is passed when from the constructor and
  // it should not be nil.
  if (this->connection_.is_nil())
    {
      ACE_ERROR_RETURN((LM_ERROR,
                        "(%P|%t) ERROR: SimpleTcpReceiveStrategy::reset  previous connection "
                        "should not be nil.\n"),
                       -1);
    }
  
  if (this->connection_.in () == connection)
    {
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
  connection->_add_ref ();
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
                                       ACE_Event_Handler::READ_MASK) == -1)
    {
      // Take back the "copy" we made.
      this->connection_->_remove_ref();
      ACE_ERROR_RETURN((LM_ERROR,
                        "(%P|%t) ERROR: SimpleTcpConnection can't register with "
                        "reactor\n"),
                       -1);
    }

  return 0;
}


void
TAO::DCPS::SimpleTcpReceiveStrategy::stop_i()
{
  DBG_ENTRY("SimpleTcpReceiveStrategy","stop_i");

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

int 
TAO::DCPS::SimpleTcpReceiveStrategy::relink ()
{
  DBG_ENTRY("SimpleTcpReceiveStrategy","relink");
  return this->connection_->reconnect ();
}



