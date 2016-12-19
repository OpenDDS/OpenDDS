/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "TransportQueueElement.h"
#include "EntryExit.h"
#include "TransportCustomizedElement.h"
#include "dds/DCPS/DataSampleHeader.h"

#if !defined (__ACE_INLINE__)
# include "TransportQueueElement.inl"
#endif /* !__ACE_INLINE__ */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

TransportQueueElement::~TransportQueueElement()
{
  DBG_ENTRY_LVL("TransportQueueElement", "~TransportQueueElement", 6);
}

bool
TransportQueueElement::requires_exclusive_packet() const
{
  DBG_ENTRY_LVL("TransportQueueElement", "requires_exclusive_packet", 6);
  return false;
}

bool
TransportQueueElement::is_control(RepoId /*pub_id*/) const
{
  DBG_ENTRY_LVL("TransportQueueElement", "is_control", 6);
  return false;
}

ElementPair
TransportQueueElement::fragment(size_t size)
{
  ACE_Message_Block* head;
  ACE_Message_Block* tail;
  DataSampleHeader::split(*msg(), size, head, tail);

  TransportCustomizedElement* frag = TransportCustomizedElement::alloc(0, true);
  frag->set_publication_id(publication_id());
  frag->set_msg(head);

  TransportCustomizedElement* rest =
    TransportCustomizedElement::alloc(this, true);
  rest->set_msg(tail);

  return ElementPair(frag, rest);
}

ACE_Message_Block*
TransportQueueElement::clone_mb(const ACE_Message_Block* msg,
                                MessageBlockAllocator* mb_allocator,
                                DataBlockAllocator* db_allocator)
{
  ACE_Message_Block* cur_block = const_cast<ACE_Message_Block*>(msg);
  ACE_Message_Block* head_copy = 0;
  ACE_Message_Block* cur_copy  = 0;
  ACE_Message_Block* prev_copy = 0;
  // deep copy sample data
  while (cur_block != 0) {
    ACE_NEW_MALLOC_RETURN(cur_copy,
                          static_cast<ACE_Message_Block*>(
                          mb_allocator->malloc(sizeof(ACE_Message_Block))),
                          ACE_Message_Block(cur_block->capacity(),
                                            ACE_Message_Block::MB_DATA,
                                            0, //cont
                                            0, //data
                                            0, //alloc_strategy
                                            0, //locking_strategy
                                            ACE_DEFAULT_MESSAGE_BLOCK_PRIORITY,
                                            ACE_Time_Value::zero,
                                            ACE_Time_Value::max_time,
                                            db_allocator,
                                            mb_allocator),
                          0);

    cur_copy->copy(cur_block->base(), cur_block->size());
    cur_copy->rd_ptr(cur_copy->base() +
                     (cur_block->rd_ptr() - cur_block->base()));
    cur_copy->wr_ptr(cur_copy->base() +
                     (cur_block->wr_ptr() - cur_block->base()));

    if (head_copy == 0) {
      head_copy = cur_copy;
    } else {
      prev_copy->cont(cur_copy);
    }

    prev_copy = cur_copy;

    cur_block = cur_block->cont();
  }

  return head_copy;
}

TransportQueueElement::MatchCriteria::~MatchCriteria()
{
}

TransportQueueElement::MatchOnPubId::~MatchOnPubId()
{
}

TransportQueueElement::MatchOnDataPayload::~MatchOnDataPayload()
{
}

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
