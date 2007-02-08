// -*- C++ -*-
//
// $Id$

#include "ReliableMulticast_pch.h"
#include "ReliableMulticastDataLink.h"
#include "ReliableMulticastTransportImpl.h"
#include "dds/DCPS/transport/framework/TransportReactorTask.h"
#include <iostream>

#if !defined (__ACE_INLINE__)
#include "ReliableMulticastDataLink.inl"
#endif /* __ACE_INLINE__ */

TAO::DCPS::ReliableMulticastDataLink::ReliableMulticastDataLink(
  TransportReactorTask_rch& reactor_task,
  const ACE_INET_Addr& multicast_group_address,
  TAO::DCPS::ReliableMulticastTransportImpl& transport_impl
  )
  : TAO::DCPS::DataLink(&transport_impl)
  , reactor_task_(reactor_task)
  , transport_impl_(&transport_impl)
{
  transport_impl_->_add_ref();
}

bool
TAO::DCPS::ReliableMulticastDataLink::connect(bool is_publisher)
{
  std::cout << "is_publisher == " << is_publisher << std::endl;
  return false;
}

void
TAO::DCPS::ReliableMulticastDataLink::stop_i()
{
}
