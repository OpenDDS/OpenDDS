/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "EntryExit.h"

ACE_INLINE
OpenDDS::DCPS::TransportSendControlElement::TransportSendControlElement
(int                    initial_count,
 RepoId                 publisher_id,
 TransportSendListener* listener,
 const DataSampleHeader& header,
 ACE_Message_Block*     msg_block,
 TransportSendControlElementAllocator* allocator)
  : TransportQueueElement(initial_count),
    publisher_id_(publisher_id),
    listener_(listener),
    header_(header),
    msg_(msg_block),
    allocator_(allocator)
{
  DBG_ENTRY_LVL("TransportSendControlElement","TransportSendControlElement",6);
}


ACE_INLINE
bool
OpenDDS::DCPS::TransportSendControlElement::owned_by_transport()
{
  return false;
}

ACE_INLINE
OpenDDS::DCPS::SequenceNumber
OpenDDS::DCPS::TransportSendControlElement::sequence() const
{
  return this->header_.sequence_;
}
