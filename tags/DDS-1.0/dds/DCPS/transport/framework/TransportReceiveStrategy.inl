// -*- C++ -*-
//
// $Id$
#include "EntryExit.h"
#include "ace/OS.h"


ACE_INLINE int
OpenDDS::DCPS::TransportReceiveStrategy::start()
{
  DBG_ENTRY_LVL("TransportReceiveStrategy","start",5);
  return this->start_i();
}


ACE_INLINE void
OpenDDS::DCPS::TransportReceiveStrategy::stop()
{
  DBG_ENTRY_LVL("TransportReceiveStrategy","stop",5);
  this->stop_i();
}


ACE_INLINE size_t
OpenDDS::DCPS::TransportReceiveStrategy::successor_index(size_t index) const
{
  return ++index % RECEIVE_BUFFERS;
}


ACE_INLINE void
OpenDDS::DCPS::TransportReceiveStrategy::relink (bool)
{
  // The subsclass needs implement this function for re-establishing
  // the link upon recv failure.
}
