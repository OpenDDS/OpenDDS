// -*- C++ -*-
//
// $Id$
#include  "SimpleMcast_pch.h"
#include  "SimpleMcastSendStrategy.h"

#if !defined (__ACE_INLINE__)
#include "SimpleMcastSendStrategy.inl"
#endif /* __ACE_INLINE__ */

TAO::DCPS::SimpleMcastSendStrategy::~SimpleMcastSendStrategy()
{
  DBG_ENTRY_LVL("SimpleMcastSendStrategy","~SimpleMcastSendStrategy",5);
}


ssize_t
TAO::DCPS::SimpleMcastSendStrategy::send_bytes(const iovec iov[], int n, int& bp)
{
  DBG_ENTRY_LVL("SimpleMcastSendStrategy","send_bytes",5);

  return this->non_blocking_send (iov, n, bp);
}

ACE_HANDLE 
TAO::DCPS::SimpleMcastSendStrategy::get_handle ()
{
  return this->socket_->get_handle();
}


ssize_t 
TAO::DCPS::SimpleMcastSendStrategy::send_bytes_i (const iovec iov[], int n)
{
  return this->socket_->send_bytes(iov, n, this->addr_);
}



