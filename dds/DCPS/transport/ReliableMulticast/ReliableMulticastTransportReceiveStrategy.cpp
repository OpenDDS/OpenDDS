// -*- C++ -*-
//
// $Id$

#include "ReliableMulticast_pch.h"
#include "ReliableMulticastTransportReceiveStrategy.h"

#if !defined (__ACE_INLINE__)
#include "ReliableMulticastTransportReceiveStrategy.inl"
#endif /* __ACE_INLINE__ */

ssize_t
TAO::DCPS::ReliableMulticastTransportReceiveStrategy::receive_bytes(
  iovec iov[],
  int n,
  ACE_INET_Addr& remote_address
  )
{
  return -1;
}

void
TAO::DCPS::ReliableMulticastTransportReceiveStrategy::deliver_sample(
  ReceivedDataSample& sample,
  const ACE_INET_Addr& remote_address
  )
{
}

int
TAO::DCPS::ReliableMulticastTransportReceiveStrategy::start_i()
{
  return -1;
}

void
TAO::DCPS::ReliableMulticastTransportReceiveStrategy::stop_i()
{
}
