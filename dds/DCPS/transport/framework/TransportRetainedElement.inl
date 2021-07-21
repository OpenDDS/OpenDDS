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
    ACE_Message_Block*                 message,
    const RepoId&                      pubId
) : TransportQueueElement(1),
    msg_(message),
    publication_id_(pubId),
    mb_allocator_(0),
    db_allocator_(0),
    is_duplicate_(true)
{
}

ACE_INLINE
OpenDDS::DCPS::TransportRetainedElement::TransportRetainedElement(
    const ACE_Message_Block*           message,
    const RepoId&                      pubId,
    MessageBlockAllocator*             mb_allocator,
    DataBlockAllocator*                db_allocator
) : TransportQueueElement(1),
    msg_(message ? TransportQueueElement::clone_mb(message,
                                                   mb_allocator,
                                                   db_allocator) : 0),
    publication_id_(pubId),
    mb_allocator_(mb_allocator),
    db_allocator_(db_allocator),
    is_duplicate_(false)
{
}

ACE_INLINE
OpenDDS::DCPS::TransportRetainedElement::TransportRetainedElement(
  const TransportRetainedElement& source
) : TransportQueueElement(1),
    msg_(source.duplicate_msg()),
    publication_id_(source.publication_id()),
    mb_allocator_(source.mb_allocator_),
    db_allocator_(source.db_allocator_),
    is_duplicate_(source.is_duplicate_)
{
  DBG_ENTRY_LVL("TransportRetainedElement", "TransportRetainedElement", 6);
}


ACE_INLINE
bool
OpenDDS::DCPS::TransportRetainedElement::owned_by_transport()
{
  return !is_duplicate_;
}

ACE_INLINE
bool
OpenDDS::DCPS::TransportRetainedElement::is_retained_replaced() const
{
  return !is_duplicate_;
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
