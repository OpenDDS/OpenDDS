// -*- C++ -*-
//
// $Id$
#include  "SimpleMcast_pch.h"
#include  "SimpleMcastSocket.h"


#if !defined (__ACE_INLINE__)
#include "SimpleMcastSocket.inl"
#endif /* __ACE_INLINE__ */


TAO::DCPS::SimpleMcastSocket::~SimpleMcastSocket()
{
  DBG_ENTRY_LVL("SimpleMcastSocket","~SimpleMcastSocket",5);
}


ACE_HANDLE
TAO::DCPS::SimpleMcastSocket::get_handle() const
{
  DBG_ENTRY_LVL("SimpleMcastSocket","get_handle",5);
  return this->socket_.get_handle();
}


int
TAO::DCPS::SimpleMcastSocket::handle_input(ACE_HANDLE)
{
  DBG_ENTRY_LVL("SimpleMcastSocket","handle_input",5);

  if (!this->receive_strategy_.is_nil())
    {
      return this->receive_strategy_->handle_input();
    }

  return 0;
}


int
TAO::DCPS::SimpleMcastSocket::handle_close(ACE_HANDLE, ACE_Reactor_Mask)
{
  DBG_ENTRY_LVL("SimpleMcastSocket","handle_close",5);

  return 0;
}


int
TAO::DCPS::SimpleMcastSocket::set_receive_strategy
                                     (TransportReceiveStrategy* strategy,
                                      TransportReactorTask*     reactor_task)
{
  DBG_ENTRY_LVL("SimpleMcastSocket","set_receive_strategy",5);

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
                        "(%P|%t) ERROR: SimpleMcastSocket cannot register with "
                        "reactor\n"),
                       -1);
    }

  return 0;
}


void
TAO::DCPS::SimpleMcastSocket::remove_receive_strategy()
{
  DBG_ENTRY_LVL("SimpleMcastSocket","remove_receive_strategy",5);

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
