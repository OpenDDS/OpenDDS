// -*- C++ -*-
//
// $Id$
#include "SimpleUnreliableDgram_pch.h"
#include "SimpleUnreliableDgramReceiveStrategy.h"
#include "dds/DCPS/transport/framework/EntryExit.h"

#if !defined (__ACE_INLINE__)
#include "SimpleUnreliableDgramReceiveStrategy.inl"
#endif /* __ACE_INLINE__ */


TAO::DCPS::SimpleUnreliableDgramReceiveStrategy::~SimpleUnreliableDgramReceiveStrategy()
{
  DBG_ENTRY_LVL("SimpleUnreliableDgramReceiveStrategy","~SimpleUnreliableDgramReceiveStrategy",5);
}


ssize_t
TAO::DCPS::SimpleUnreliableDgramReceiveStrategy::receive_bytes(iovec          iov[],
                                                   int            n,
                                                   ACE_INET_Addr& remote_addr)
{
  DBG_ENTRY_LVL("SimpleUnreliableDgramReceiveStrategy","receive_bytes",5);
  return this->socket_->receive_bytes(iov, n, remote_addr);
}


int
TAO::DCPS::SimpleUnreliableDgramReceiveStrategy::start_i()
{
  DBG_ENTRY_LVL("SimpleUnreliableDgramReceiveStrategy","start_i",5);
  return this->socket_->set_receive_strategy(this,this->task_.in());
}


void
TAO::DCPS::SimpleUnreliableDgramReceiveStrategy::stop_i()
{
  DBG_ENTRY_LVL("SimpleUnreliableDgramReceiveStrategy","stop_i",5);

  this->socket_->remove_receive_strategy();

  this->task_ = 0;
  this->socket_ = 0;
  this->transport_ = 0;
}


void
TAO::DCPS::SimpleUnreliableDgramReceiveStrategy::deliver_sample
                                        (ReceivedDataSample&  sample,
                                         const ACE_INET_Addr& remote_address)
{
  DBG_ENTRY_LVL("SimpleUnreliableDgramReceiveStrategy","deliver_sample",5);

  this->transport_->deliver_sample(sample, remote_address);
}

