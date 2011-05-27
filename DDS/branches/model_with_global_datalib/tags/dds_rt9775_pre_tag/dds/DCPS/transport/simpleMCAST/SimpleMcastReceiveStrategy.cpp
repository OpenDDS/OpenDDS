// -*- C++ -*-
//
// $Id$
#include  "SimpleMcast_pch.h"
#include  "SimpleMcastReceiveStrategy.h"
#include  "SimpleMcastSocket.h"
#include  "SimpleMcastTransport.h"
#include  "dds/DCPS/transport/framework/EntryExit.h"
#include  "dds/DCPS/transport/framework/TransportReactorTask.h"

#if !defined (__ACE_INLINE__)
#include "SimpleMcastReceiveStrategy.inl"
#endif /* __ACE_INLINE__ */


TAO::DCPS::SimpleMcastReceiveStrategy::~SimpleMcastReceiveStrategy()
{
  DBG_ENTRY_LVL("SimpleMcastReceiveStrategy","~SimpleMcastReceiveStrategy",5);
}


ssize_t
TAO::DCPS::SimpleMcastReceiveStrategy::receive_bytes(iovec          iov[],
                                                     int            n,
                                                     ACE_INET_Addr& remote_address)
{
  DBG_ENTRY_LVL("SimpleMcastReceiveStrategy","receive_bytes",5);
  return this->socket_->receive_bytes(iov, n, remote_address);
}


int
TAO::DCPS::SimpleMcastReceiveStrategy::start_i()
{
  DBG_ENTRY_LVL("SimpleMcastReceiveStrategy","start_i",5);
  return this->socket_->set_receive_strategy(this,this->task_.in());
}


void
TAO::DCPS::SimpleMcastReceiveStrategy::stop_i()
{
  DBG_ENTRY_LVL("SimpleMcastReceiveStrategy","stop_i",5);

  this->socket_->remove_receive_strategy();

  this->task_ = 0;
  this->socket_ = 0;
  this->transport_ = 0;
}


void
TAO::DCPS::SimpleMcastReceiveStrategy::deliver_sample
                                        (ReceivedDataSample&  sample,
                                         const ACE_INET_Addr& remote_address)
{
  DBG_ENTRY_LVL("SimpleMcastReceiveStrategy","deliver_sample",5);

  this->transport_->deliver_sample(sample, remote_address);
}

