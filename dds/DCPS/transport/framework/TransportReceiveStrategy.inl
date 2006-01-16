// -*- C++ -*-
//
// $Id$
#include  "EntryExit.h"
#include  "ace/OS.h"


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

