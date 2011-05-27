// -*- C++ -*-
//
// $Id$
#include  "DCPS/DdsDcps_pch.h"
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
  DBG_ENTRY("SimpleUdpReceiveStrategy","~SimpleUdpReceiveStrategy");
}


ssize_t
TAO::DCPS::SimpleUdpReceiveStrategy::receive_bytes(iovec          iov[],
                                                   int            n,
                                                   ACE_INET_Addr& remote_addr)
{
  DBG_ENTRY("SimpleUdpReceiveStrategy","receive_bytes");
  return this->socket_->receive_bytes(iov, n, remote_addr);
}


int
TAO::DCPS::SimpleUdpReceiveStrategy::start_i()
{
  DBG_ENTRY("SimpleUdpReceiveStrategy","start_i");
  return this->socket_->set_receive_strategy(this,this->task_.in());
}


void
TAO::DCPS::SimpleUdpReceiveStrategy::stop_i()
{
  DBG_ENTRY("SimpleUdpReceiveStrategy","stop_i");

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
  DBG_ENTRY("SimpleUdpReceiveStrategy","deliver_sample");

  this->transport_->deliver_sample(sample, remote_address);
}

