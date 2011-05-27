// -*- C++ -*-
//
// $Id$
#include  "dds/DCPS/transport/framework/TransportReceiveStrategy.h"
#include  "dds/DCPS/transport/framework/TransportReactorTask.h"
#include  "ace/Reactor.h"
#include  "dds/DCPS/transport/framework/EntryExit.h"


ACE_INLINE
TAO::DCPS::SimpleUdpSocket::SimpleUdpSocket()
{
  DBG_ENTRY("SimpleUdpSocket","SimpleUdpSocket");
}



ACE_INLINE int
TAO::DCPS::SimpleUdpSocket::open_socket(const ACE_INET_Addr& local_address)
{
  DBG_ENTRY("SimpleUdpSocket","open_socket");

  this->local_address_ = local_address;
  return this->socket_.open(this->local_address_);
}


ACE_INLINE void
TAO::DCPS::SimpleUdpSocket::close_socket()
{
  DBG_ENTRY("SimpleUdpSocket","close_socket");

  // Make sure that no other thread is send()'ing to the socket_ right now.
  GuardType guard(this->lock_);
  this->socket_.close();

  this->remove_receive_strategy();
}


ACE_INLINE int
TAO::DCPS::SimpleUdpSocket::set_receive_strategy
                                     (TransportReceiveStrategy* strategy,
                                      TransportReactorTask*     reactor_task)
{
  DBG_ENTRY("SimpleUdpSocket","set_receive_strategy");

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
                        "(%P|%t) ERROR: SimpleUdpSocket cannot register with "
                        "reactor\n"),
                       -1);
    }

  return 0;
}


ACE_INLINE void
TAO::DCPS::SimpleUdpSocket::remove_receive_strategy()
{
  DBG_ENTRY("SimpleUdpSocket","remove_receive_strategy");

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


ACE_INLINE ssize_t
TAO::DCPS::SimpleUdpSocket::send_bytes(const iovec iov[],
                                       int   n,
                                       const ACE_INET_Addr& remote_address)
{
  DBG_ENTRY("SimpleUdpSocket","send_bytes");

  // Protect the socket from multiple threads attempting to send at once.
  GuardType guard(this->lock_);

  return this->socket_.send(iov,n,remote_address);
}


ACE_INLINE ssize_t
TAO::DCPS::SimpleUdpSocket::receive_bytes(iovec iov[],
                                          int   n,
                                          ACE_INET_Addr& remote_address)
{
  DBG_ENTRY("SimpleUdpSocket","receive_bytes");

  return this->socket_.recv(iov, n, remote_address);
}

