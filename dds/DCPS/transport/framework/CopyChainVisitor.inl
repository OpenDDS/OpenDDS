/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "EntryExit.h"

ACE_INLINE
OpenDDS::DCPS::CopyChainVisitor::CopyChainVisitor(
  BasicQueue<TransportQueueElement>& target,
  TransportRetainedElementAllocator* allocator
) : target_( target), allocator_( allocator), status_( 0)
{
  DBG_ENTRY_LVL("CopyChainVisitor","CopyChainVisitor",6);
}

ACE_INLINE
int
OpenDDS::DCPS::CopyChainVisitor::status() const
{
  return this->status_;
}

