// -*- C++ -*-
//
// $Id$
#include  "SimpleUdp_pch.h"
#include  "SimpleUdpReceiveStrategy.h"
#include  "SimpleUdpSocket.h"
#include  "SimpleUdpTransport.h"
#include  "dds/DCPS/transport/framework/EntryExit.h"
#include  "dds/DCPS/transport/framework/TransportReactorTask.h"

#if !defined (__ACE_INLINE__)
#include "SimpleUdpReceiveStrategy.inl"
#endif /* __ACE_INLINE__ */


TAO::DCPS::SimpleUdpReceiveStrategy::~SimpleUdpReceiveStrategy()
{
  DBG_ENTRY_LVL("SimpleUdpReceiveStrategy","~SimpleUdpReceiveStrategy",5);
}


ssize_t
TAO::DCPS::SimpleUdpReceiveStrategy::receive_bytes(iovec          iov[],
                                                   int            n,
                                                   ACE_INET_Addr& remote_addr)
{
  DBG_ENTRY_LVL("SimpleUdpReceiveStrategy","receive_bytes",5);
  return this->socket_->receive_bytes(iov, n, remote_addr);
}


int
TAO::DCPS::SimpleUdpReceiveStrategy::start_i()
{
  DBG_ENTRY_LVL("SimpleUdpReceiveStrategy","start_i",5);
  return this->socket_->set_receive_strategy(this,this->task_.in());
}


void
TAO::DCPS::SimpleUdpReceiveStrategy::stop_i()
{
  DBG_ENTRY_LVL("SimpleUdpReceiveStrategy","stop_i",5);

  this->socket_->remove_receive_strategy();

  this->task_ = 0;
  this->socket_ = 0;
  this->transport_ = 0;
}


void
TAO::DCPS::SimpleUdpReceiveStrategy::deliver_sample
                                        (ReceivedDataSample&  sample,
                                         const ACE_INET_Addr& remote_address)
{
  DBG_ENTRY_LVL("SimpleUdpReceiveStrategy","deliver_sample",5);

  this->transport_->deliver_sample(sample, remote_address);
}

