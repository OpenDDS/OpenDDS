// -*- C++ -*-
//
// $Id$
#include  "dds/DCPS/transport/framework/TransportReceiveStrategy.h"
#include  "dds/DCPS/transport/framework/TransportReactorTask.h"
#include  "ace/Reactor.h"
#include  "dds/DCPS/transport/framework/EntryExit.h"


ACE_INLINE
TAO::DCPS::SimpleMcastSocket::SimpleMcastSocket()
{
  DBG_ENTRY_LVL("SimpleMcastSocket","SimpleMcastSocket",5);
}



ACE_INLINE int
TAO::DCPS::SimpleMcastSocket::open(const ACE_INET_Addr& local_address,
                                   const ACE_INET_Addr& multicast_group_address,
                                   bool receiver)
{
  DBG_ENTRY_LVL("SimpleMcastSocket","open",5);

  int result = -1;

  this->local_address_ = local_address;
  this->multicast_group_address_ = multicast_group_address;
  if (!receiver)
  {
    result = this->socket_.ACE_SOCK_Dgram::open(this->local_address_);
  }
  else
  {
    result = this->socket_.join(this->multicast_group_address_);
  }

  return result;
}


ACE_INLINE void
TAO::DCPS::SimpleMcastSocket::close()
{
  DBG_ENTRY_LVL("SimpleMcastSocket","close",5);

  // Make sure that no other thread is send()'ing to the socket_ right now.
  GuardType guard(this->lock_);

  this->socket_.close();

  this->remove_receive_strategy();
}


ACE_INLINE ssize_t
TAO::DCPS::SimpleMcastSocket::send_bytes(const iovec iov[],
                                         int   n,
                                         const ACE_INET_Addr& multicast_group_address)
{
  DBG_ENTRY_LVL("SimpleMcastSocket","send_bytes",5);

  ACE_UNUSED_ARG(multicast_group_address);

  // Protect the socket from multiple threads attempting to send at once.
  GuardType guard(this->lock_);

  return this->socket_.ACE_SOCK_Dgram::send(iov, n, multicast_group_address_);
}


ACE_INLINE ssize_t
TAO::DCPS::SimpleMcastSocket::receive_bytes(iovec iov[],
                                            int   n,
                                            ACE_INET_Addr& multicast_group_address)
{
  DBG_ENTRY_LVL("SimpleMcastSocket","receive_bytes",5);

  // We always receive on our internally-known multicast group address.
  multicast_group_address = multicast_group_address_;

  int result = this->socket_.recv(iov, n, multicast_group_address);
  return result;
}

