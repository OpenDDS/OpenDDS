// -*- C++ -*-
//
// $Id$
#include "SimpleUnreliableDgram_pch.h"
#include "SimpleUnreliableDgramSendStrategy.h"


#if !defined (__ACE_INLINE__)
#include "SimpleUnreliableDgramSendStrategy.inl"
#endif /* __ACE_INLINE__ */


TAO::DCPS::SimpleUnreliableDgramSendStrategy::~SimpleUnreliableDgramSendStrategy()
{
  DBG_ENTRY_LVL("SimpleUnreliableDgramSendStrategy","~SimpleUnreliableDgramSendStrategy",5);
}


ssize_t
TAO::DCPS::SimpleUnreliableDgramSendStrategy::send_bytes(const iovec iov[], int n, int& bp)
{
  DBG_ENTRY_LVL("SimpleUnreliableDgramSendStrategy","send_bytes",5);

  return this->non_blocking_send (iov, n, bp);
}

ACE_HANDLE 
TAO::DCPS::SimpleUnreliableDgramSendStrategy::get_handle ()
{
  return this->socket_->get_handle();
}


ssize_t 
TAO::DCPS::SimpleUnreliableDgramSendStrategy::send_bytes_i (const iovec iov[], int n)
{
  // It's unlikely the SimpleUdp send bytes fail because of backpressure,
  // but the SimpleMcast is easy to have backpressure. In either protocol,
  // the backpressure timeout will be notified to the datawriter.
  return this->socket_->send_bytes(iov, n, this->remote_address_);
}

