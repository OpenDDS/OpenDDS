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
  DBG_ENTRY_LVL("SimpleUdpSocket","SimpleUdpSocket",5);
}



ACE_INLINE int
TAO::DCPS::SimpleUdpSocket::open_socket(const ACE_INET_Addr& local_address)
{
  DBG_ENTRY_LVL("SimpleUdpSocket","open_socket",5);

  this->local_address_ = local_address;
  return this->socket_.open(this->local_address_);
}


ACE_INLINE void
TAO::DCPS::SimpleUdpSocket::close_socket()
{
  DBG_ENTRY_LVL("SimpleUdpSocket","close_socket",5);

  // Make sure that no other thread is send()'ing to the socket_ right now.
  GuardType guard(this->lock_);
  this->socket_.close();

  this->remove_receive_strategy();
}


ACE_INLINE ssize_t
TAO::DCPS::SimpleUdpSocket::send_bytes(const iovec iov[],
                                       int   n,
                                       const ACE_INET_Addr& remote_address)
{
  DBG_ENTRY_LVL("SimpleUdpSocket","send_bytes",5);

  // Protect the socket from multiple threads attempting to send at once.
  GuardType guard(this->lock_);

  return this->socket_.send(iov,n,remote_address);
}


ACE_INLINE ssize_t
TAO::DCPS::SimpleUdpSocket::receive_bytes(iovec iov[],
                                          int   n,
                                          ACE_INET_Addr& remote_address)
{
  DBG_ENTRY_LVL("SimpleUdpSocket","receive_bytes",5);

  return this->socket_.recv(iov, n, remote_address);
}

