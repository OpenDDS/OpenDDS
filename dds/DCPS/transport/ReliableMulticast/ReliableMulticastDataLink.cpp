// -*- C++ -*-
//
// $Id$

#include "ReliableMulticast_pch.h"
#include "ReliableMulticastDataLink.h"
#include "ReliableMulticastTransportImpl.h"
#include "ReliableMulticastThreadSynchResource.h"
#include "dds/DCPS/transport/framework/TransportReactorTask.h"

#if !defined (__ACE_INLINE__)
#include "ReliableMulticastDataLink.inl"
#endif /* __ACE_INLINE__ */

TAO::DCPS::ReliableMulticastDataLink::ReliableMulticastDataLink(
  TransportReactorTask_rch& reactor_task,
  ReliableMulticastTransportConfiguration& configuration,
  const ACE_INET_Addr& multicast_group_address,
  TAO::DCPS::ReliableMulticastTransportImpl& transport_impl
  )
  : TAO::DCPS::DataLink(&transport_impl)
  , multicast_group_address_(multicast_group_address)
  , is_publisher_(false)
  , reactor_task_(reactor_task)
  , transport_impl_(&transport_impl)
  , receive_strategy_(*this)
  , send_strategy_(configuration, new TAO::DCPS::ReliableMulticastThreadSynchResource)
{
  transport_impl_->_add_ref();
}

bool
TAO::DCPS::ReliableMulticastDataLink::connect(bool is_publisher)
{
  if (is_publisher)
  {
    send_strategy_.configure(reactor_task_->get_reactor(), multicast_group_address_);
  }
  else
  {
    receive_strategy_.configure(reactor_task_->get_reactor(), multicast_group_address_);
  }
  start(&send_strategy_, &receive_strategy_);
  return true;
}

void
TAO::DCPS::ReliableMulticastDataLink::stop_i()
{
  send_strategy_.teardown();
  receive_strategy_.teardown();
}
