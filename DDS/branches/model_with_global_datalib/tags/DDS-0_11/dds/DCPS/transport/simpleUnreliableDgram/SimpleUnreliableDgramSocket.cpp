// -*- C++ -*-
//
// $Id$
#include  "SimpleUnreliableDgram_pch.h"
#include  "SimpleUnreliableDgramSocket.h"
#include  "ace/Reactor.h"


#if !defined (__ACE_INLINE__)
#include "SimpleUnreliableDgramSocket.inl"
#endif /* __ACE_INLINE__ */


TAO::DCPS::SimpleUnreliableDgramSocket::~SimpleUnreliableDgramSocket()
{
  DBG_ENTRY_LVL("SimpleUnreliableDgramSocket","~SimpleUnreliableDgramSocket",5);
}


int
TAO::DCPS::SimpleUnreliableDgramSocket::handle_input(ACE_HANDLE)
{
  DBG_ENTRY_LVL("SimpleUnreliableDgramSocket","handle_input",5);

  if (!this->receive_strategy_.is_nil())
    {
      return this->receive_strategy_->handle_input();
    }

  return 0;
}


int
TAO::DCPS::SimpleUnreliableDgramSocket::handle_close(ACE_HANDLE, ACE_Reactor_Mask)
{
  DBG_ENTRY_LVL("SimpleUnreliableDgramSocket","handle_close",5);

  return 0;
}


int
TAO::DCPS::SimpleUnreliableDgramSocket::set_receive_strategy
                                     (TransportReceiveStrategy* strategy,
                                      TransportReactorTask*     reactor_task)
{
  DBG_ENTRY_LVL("SimpleUnreliableDgramSocket","set_receive_strategy",5);

  // Keep a "copy" of the reference for ourselves
  strategy->_add_ref();
  this->receive_strategy_ = strategy;

  // Keep a "copy" of the reference for ourselves
  reactor_task->_add_ref();
  this->task_ = reactor_task;

  // Register with the reactor.
  if (this->task_->get_reactor()->register_handler
                                      (this,
                                       ACE_Event_Handler::READ_MASK) == -1)
    {
      // Drop the reference to the receive strategy
      this->receive_strategy_ = 0;

      // Drop the reference to the reactor task.
      this->task_ = 0;

      ACE_ERROR_RETURN((LM_ERROR,
                        "(%P|%t) ERROR: SimpleUnreliableDgramSocket cannot register with "
                        "reactor\n"),
                       -1);
    }

  return 0;
}


void
TAO::DCPS::SimpleUnreliableDgramSocket::remove_receive_strategy()
{
  DBG_ENTRY_LVL("SimpleUnreliableDgramSocket","remove_receive_strategy",5);

  if (!this->task_.is_nil())
    {
      // Unregister from the reactor.
      this->task_->get_reactor()->remove_handler(this,
                                                 ACE_Event_Handler::READ_MASK |
                                                 ACE_Event_Handler::DONT_CALL);
    }

  // Drop the reference to the receive strategy
  this->receive_strategy_ = 0;

  // Drop the reference to the reactor task
  this->task_ = 0;
}
