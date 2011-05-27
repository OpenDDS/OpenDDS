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
  DBG_ENTRY("SimpleUdpSendStrategy","~SimpleUdpSendStrategy");
}


ssize_t
TAO::DCPS::SimpleUdpSendStrategy::send_bytes(const iovec iov[], int n, int& bp)
{
  DBG_ENTRY("SimpleUdpSendStrategy","send_bytes");

  // We never experience backpressure with UDP so we never set the bp flag
  // to true.
  ACE_UNUSED_ARG(bp);

  return this->socket_->send_bytes(iov, n, this->addr_);
}

