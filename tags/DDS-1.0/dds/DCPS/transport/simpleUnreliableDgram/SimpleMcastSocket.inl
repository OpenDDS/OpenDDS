// -*- C++ -*-
//
// $Id$
#include "dds/DCPS/transport/framework/EntryExit.h"


ACE_INLINE
OpenDDS::DCPS::SimpleMcastSocket::SimpleMcastSocket()
{
  DBG_ENTRY_LVL("SimpleMcastSocket","SimpleMcastSocket",5);
}


ACE_INLINE
ACE_HANDLE
OpenDDS::DCPS::SimpleMcastSocket::get_handle() const
{
  DBG_ENTRY_LVL("SimpleMcastSocket","get_handle",5);
  return this->socket_.get_handle();
}


ACE_INLINE int
OpenDDS::DCPS::SimpleMcastSocket::open_socket (ACE_INET_Addr& local_address,
                                   const ACE_INET_Addr& multicast_group_address,
                                   bool receiver)
{
  DBG_ENTRY_LVL("SimpleMcastSocket","open",5);

  GuardType guard(this->lock_);

  this->multicast_group_address_ = multicast_group_address;

  int result = -1;

  if (!receiver)
  {
    result = this->socket_.ACE_SOCK_Dgram::open(local_address);
  }
  else
  {
    result = this->socket_.join(multicast_group_address);
  }

  if (result == 0)
  {
    ACE_INET_Addr address;
    if (this->socket_.get_local_addr (address) != 0)
    {
      ACE_ERROR_RETURN ((LM_ERROR,
        ACE_TEXT ("(%P|%t) ERROR: SimpleMcastSocket::open_socket ")
        ACE_TEXT ("- %p"),
        ACE_TEXT ("cannot get local addr\n")),
        -1);
    }
    local_address.set_port_number (address.get_port_number ());
    this->local_address_ = local_address;
  }

  return result;
}



ACE_INLINE void
OpenDDS::DCPS::SimpleMcastSocket::close_socket ()
{
  DBG_ENTRY_LVL("SimpleMcastSocket","close",5);

  // Make sure that no other thread is send()'ing to the socket_ right now.
  GuardType guard(this->lock_);

  this->socket_.close();

  this->remove_receive_strategy();
}


ACE_INLINE ssize_t
OpenDDS::DCPS::SimpleMcastSocket::send_bytes(const iovec iov[],
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
OpenDDS::DCPS::SimpleMcastSocket::receive_bytes(iovec iov[],
                                            int   n,
                                            ACE_INET_Addr& multicast_group_address)
{
  DBG_ENTRY_LVL("SimpleMcastSocket","receive_bytes",5);

  // We always receive on our internally-known multicast group address.
  multicast_group_address = multicast_group_address_;

  int result = this->socket_.recv(iov, n, multicast_group_address);
  return result;
}

