// -*- C++ -*-
//
// $Id$
#include  "EntryExit.h"
#include  "ace/OS.h"

ACE_INLINE
TAO::DCPS::TransportReceiveStrategy::TransportReceiveStrategy()
  : receive_sample_remaining_(0),
    mb_allocator_(MESSAGE_BLOCKS),
    db_allocator_(DATA_BLOCKS),
    data_allocator_(DATA_BLOCKS),
    buffer_index_(0)
{
  DBG_ENTRY("TransportReceiveStrategy","TransportReceiveStrategy");

  if (DCPS_debug_level >= 2) 
    {
      ACE_DEBUG((LM_DEBUG,"(%P|%t) TransportReceiveStrategy-mb"
                     " Cached_Allocator_With_Overflow %x with %d chunks\n",
                      &mb_allocator_, MESSAGE_BLOCKS));
      ACE_DEBUG((LM_DEBUG,"(%P|%t) TransportReceiveStrategy-db"
                     " Cached_Allocator_With_Overflow %x with %d chunks\n",
                      &db_allocator_, DATA_BLOCKS));
      ACE_DEBUG((LM_DEBUG,"(%P|%t) TransportReceiveStrategy-data"
                     " Cached_Allocator_With_Overflow %x with %d chunks\n",
                      &data_allocator_, DATA_BLOCKS));
    }

  // No aggregate assignment possible in initializer list.  That I know
  // of anyway.
  ACE_OS::memset(this->receive_buffers_, 0x0, sizeof(this->receive_buffers_));
}



ACE_INLINE int
TAO::DCPS::TransportReceiveStrategy::start()
{
  DBG_ENTRY("TransportReceiveStrategy","start");
  return this->start_i();
}


ACE_INLINE void
TAO::DCPS::TransportReceiveStrategy::stop()
{
  DBG_ENTRY("TransportReceiveStrategy","stop");
  this->stop_i();
}


ACE_INLINE size_t
TAO::DCPS::TransportReceiveStrategy::successor_index(size_t index) const
{
  return ++index % RECEIVE_BUFFERS;
}

