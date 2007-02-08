// -*- C++ -*-
//
// $Id$

#include "ReliableMulticast_pch.h"
#include "ReliableMulticastDataLink.h"
#include "ReliableMulticastTransportImpl.h"

#if !defined (__ACE_INLINE__)
#include "ReliableMulticastDataLink.inl"
#endif /* __ACE_INLINE__ */

TAO::DCPS::ReliableMulticastDataLink::ReliableMulticastDataLink(
  const ACE_INET_Addr& multicast_group_address,
  TAO::DCPS::ReliableMulticastTransportImpl& transport_impl
  )
  : TAO::DCPS::DataLink(&transport_impl)
  , transport_impl_(&transport_impl)
{
  transport_impl_->_add_ref();
}

bool
TAO::DCPS::ReliableMulticastDataLink::connect(bool is_publisher)
{
  return false;
}
