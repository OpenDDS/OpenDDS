/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "EntryExit.h"

ACE_INLINE
OpenDDS::DCPS::CopyChainVisitor::CopyChainVisitor(
  BasicQueue<TransportQueueElement>& target,
  TransportRetainedElementAllocator* allocator,
  MessageBlockAllocator* mb_allocator,
  DataBlockAllocator* db_allocator
) : target_( target)
  , allocator_( allocator)
  , mb_allocator_(mb_allocator)
  , db_allocator_(db_allocator)
  , status_( 0)
{
  DBG_ENTRY_LVL("CopyChainVisitor","CopyChainVisitor",6);
}

ACE_INLINE
int
OpenDDS::DCPS::CopyChainVisitor::status() const
{
  return this->status_;
}

