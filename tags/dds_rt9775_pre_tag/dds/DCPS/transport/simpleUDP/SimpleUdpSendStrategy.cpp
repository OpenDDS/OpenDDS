// -*- C++ -*-
//
// $Id$
#include  "SimpleUdp_pch.h"
#include  "SimpleUdpSendStrategy.h"


#if !defined (__ACE_INLINE__)
#include "SimpleUdpSendStrategy.inl"
#endif /* __ACE_INLINE__ */


TAO::DCPS::SimpleUdpSendStrategy::~SimpleUdpSendStrategy()
{
  DBG_ENTRY_LVL("SimpleUdpSendStrategy","~SimpleUdpSendStrategy",5);
}


ssize_t
TAO::DCPS::SimpleUdpSendStrategy::send_bytes(const iovec iov[], int n, int& bp)
{
  DBG_ENTRY_LVL("SimpleUdpSendStrategy","send_bytes",5);

  return this->non_blocking_send (iov, n, bp);
}

ACE_HANDLE 
TAO::DCPS::SimpleUdpSendStrategy::get_handle ()
{
  return this->socket_->get_handle();
}


ssize_t 
TAO::DCPS::SimpleUdpSendStrategy::send_bytes_i (const iovec iov[], int n)
{
  // It's unlikely the SimpleUdp has backpressure.  
  return this->socket_->send_bytes(iov, n, this->addr_);
}

