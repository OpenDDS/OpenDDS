/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "EntryExit.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

ACE_INLINE
OpenDDS::DCPS::TransportRetainedElement::TransportRetainedElement(
    const ACE_Message_Block*           message,
    const RepoId&                      pubId,
    TransportRetainedElementAllocator* allocator,
    MessageBlockAllocator*             mb_allocator,
    DataBlockAllocator*                db_allocator
) : TransportQueueElement(1),
    msg_(0),
    publication_id_( pubId),
    allocator_( allocator),
    mb_allocator_( mb_allocator),
    db_allocator_( db_allocator)
{
  if (message != 0) {
    msg_ = TransportQueueElement::clone_mb(message,
                                           this->mb_allocator_,
                                           this->db_allocator_);
  }
}

ACE_INLINE
OpenDDS::DCPS::TransportRetainedElement::TransportRetainedElement(
  const TransportRetainedElement& source
) : TransportQueueElement(1),
    msg_( ACE_Message_Block::duplicate( source.msg())),
    publication_id_( source.publication_id()),
    allocator_( source.allocator_),
    mb_allocator_( source.mb_allocator_),
    db_allocator_( source.db_allocator_)
{
  DBG_ENTRY_LVL("TransportRetainedElement","TransportRetainedElement",6);
}


ACE_INLINE
bool
OpenDDS::DCPS::TransportRetainedElement::owned_by_transport ()
{
  return true;
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
