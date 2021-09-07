/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "EntryExit.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

ACE_INLINE
OpenDDS::DCPS::TransportReplacedElement::TransportReplacedElement
(TransportQueueElement* orig_elem,
 MessageBlockAllocator* mb_allocator,
 DataBlockAllocator* db_allocator)
  : TransportQueueElement(1)
  , mb_allocator_ (mb_allocator)
  , db_allocator_ (db_allocator)
{
  DBG_ENTRY_LVL("TransportReplacedElement", "TransportReplacedElement", 6);

  // Obtain the publisher id.
  publisher_id_ = orig_elem->publication_id();

  msg_.reset(TransportQueueElement::clone_mb(orig_elem->msg(),
                                             mb_allocator_,
                                             db_allocator_));
}

ACE_INLINE
OpenDDS::DCPS::RepoId
OpenDDS::DCPS::TransportReplacedElement::publication_id() const
{
  DBG_ENTRY_LVL("TransportReplacedElement", "publication_id", 6);
  return publisher_id_;
}

ACE_INLINE
ACE_Message_Block*
OpenDDS::DCPS::TransportReplacedElement::duplicate_msg() const
{
  DBG_ENTRY_LVL("TransportReplacedElement", "duplicate_msg", 6);
  return msg_->duplicate();
}

ACE_INLINE
const ACE_Message_Block*
OpenDDS::DCPS::TransportReplacedElement::msg() const
{
  DBG_ENTRY_LVL("TransportReplacedElement", "msg", 6);
  return msg_.get();
}

ACE_INLINE
const ACE_Message_Block*
OpenDDS::DCPS::TransportReplacedElement::msg_payload() const
{
  DBG_ENTRY_LVL("TransportReplacedElement", "msg_payload", 6);
  return msg_->cont();
}

ACE_INLINE
bool
OpenDDS::DCPS::TransportReplacedElement::owned_by_transport ()
{
  return true;
}

ACE_INLINE
bool
OpenDDS::DCPS::TransportReplacedElement::is_retained_replaced() const
{
  return true;
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
