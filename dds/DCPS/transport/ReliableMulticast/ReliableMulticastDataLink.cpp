// -*- C++ -*-
//
// $Id$

#include "ReliableMulticast_pch.h"
#include "ReliableMulticastDataLink.h"
#include "ReliableMulticastTransportImpl.h"
#include "ReliableMulticastThreadSynchResource.h"
#include "ReliableMulticastTransportConfiguration.h"

#include "dds/DCPS/transport/framework/TransportReactorTask.h"

#if !defined (__ACE_INLINE__)
#include "ReliableMulticastDataLink.inl"
#endif /* __ACE_INLINE__ */

OpenDDS::DCPS::ReliableMulticastDataLink::ReliableMulticastDataLink(
  TransportReactorTask_rch& reactor_task,
  ReliableMulticastTransportConfiguration& configuration,
  const ACE_INET_Addr& multicast_group_address,
  OpenDDS::DCPS::ReliableMulticastTransportImpl& transport_impl,
  CORBA::Long priority
  )
  : OpenDDS::DCPS::DataLink(&transport_impl, priority)
  , local_address_(configuration.local_address_)
  , multicast_group_address_(multicast_group_address)
  , sender_history_size_(configuration.sender_history_size_)
  , receiver_buffer_size_(configuration.receiver_buffer_size_)
  , is_publisher_(false)
  , reactor_task_(reactor_task)
  , transport_impl_(&transport_impl, false)
  , receive_strategy_(*this)
  , send_strategy_(configuration, new OpenDDS::DCPS::ReliableMulticastThreadSynchResource, priority)
  , running_(false)
{
}

bool
OpenDDS::DCPS::ReliableMulticastDataLink::connect(bool is_publisher)
{
  if (is_publisher)
  {
    send_strategy_.configure(
      reactor_task_->get_reactor(),
      local_address_,
      multicast_group_address_,
      sender_history_size_
      );
  }
  else
  {
    receive_strategy_.configure(
      reactor_task_->get_reactor(),
      multicast_group_address_,
      receiver_buffer_size_
      );
  }
  start(&send_strategy_, &receive_strategy_);
  running_ = true;
  return true;
}

void
OpenDDS::DCPS::ReliableMulticastDataLink::stop_i()
{
  if (running_)
  {
    send_strategy_.teardown();
    receive_strategy_.teardown();
    reactor_task_ = 0;
    running_ = false;
  }
}
