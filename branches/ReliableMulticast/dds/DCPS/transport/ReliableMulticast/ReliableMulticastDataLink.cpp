// -*- C++ -*-
//
// $Id$

#include "ReliableMulticast_pch.h"
#include "ReliableMulticastDataLink.h"
#include "ReliableMulticastTransportImpl.h"
#include "detail/PacketReceiverCallback.h"
#include "dds/DCPS/transport/framework/TransportReactorTask.h"
#include <iostream>

#if !defined (__ACE_INLINE__)
#include "ReliableMulticastDataLink.inl"
#endif /* __ACE_INLINE__ */

typedef TAO::DCPS::ReliableMulticast::detail::ReactivePacketReceiver ReactivePacketReceiver;
typedef TAO::DCPS::ReliableMulticast::detail::ReactivePacketSender ReactivePacketSender;

namespace
{
  class CallbackImpl
    : public TAO::DCPS::ReliableMulticast::detail::PacketReceiverCallback
  {
  public:
    virtual ~CallbackImpl() {}

    virtual void received_packets(
      const std::vector<TAO::DCPS::ReliableMulticast::detail::Packet>& packets
      )
    {
    }

    virtual void reliability_compromised()
    {
    }
  };

  CallbackImpl callback_;
}

TAO::DCPS::ReliableMulticastDataLink::ReliableMulticastDataLink(
  TransportReactorTask_rch& reactor_task,
  const ACE_INET_Addr& multicast_group_address,
  TAO::DCPS::ReliableMulticastTransportImpl& transport_impl
  )
  : TAO::DCPS::DataLink(&transport_impl)
  , multicast_group_address_(multicast_group_address)
  , is_publisher_(false)
  , reactor_task_(reactor_task)
  , transport_impl_(&transport_impl)
{
  transport_impl_->_add_ref();
}

bool
TAO::DCPS::ReliableMulticastDataLink::connect(bool is_publisher)
{
  if (is_publisher)
  {
    receiver_.reset();
    sender_.reset(new ReactivePacketSender(multicast_group_address_));
  }
  else
  {
    receiver_.reset(new ReactivePacketReceiver(multicast_group_address_, callback_));
    sender_.reset();
  }
  return true;
}

void
TAO::DCPS::ReliableMulticastDataLink::stop_i()
{
  receiver_.reset();
  sender_.reset();
}
