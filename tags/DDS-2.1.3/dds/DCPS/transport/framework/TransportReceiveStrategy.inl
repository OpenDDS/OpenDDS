/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "EntryExit.h"
#include "ace/OS.h"

ACE_INLINE int
OpenDDS::DCPS::TransportReceiveStrategy::start()
{
  DBG_ENTRY_LVL("TransportReceiveStrategy","start",6);
  return this->start_i();
}

ACE_INLINE void
OpenDDS::DCPS::TransportReceiveStrategy::stop()
{
  DBG_ENTRY_LVL("TransportReceiveStrategy","stop",6);
  this->stop_i();
}

ACE_INLINE const OpenDDS::DCPS::TransportHeader&
OpenDDS::DCPS::TransportReceiveStrategy::received_header() const
{
  DBG_ENTRY_LVL("TransportReceiveStrategy","received_header",6);
  return this->receive_transport_header_;
}

ACE_INLINE size_t
OpenDDS::DCPS::TransportReceiveStrategy::successor_index(size_t index) const
{
  return ++index % RECEIVE_BUFFERS;
}

ACE_INLINE void
OpenDDS::DCPS::TransportReceiveStrategy::relink(bool)
{
  // The subsclass needs implement this function for re-establishing
  // the link upon recv failure.
}
