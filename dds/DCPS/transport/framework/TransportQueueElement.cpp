/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "TransportQueueElement.h"
#include "EntryExit.h"

#if !defined (__ACE_INLINE__)
# include "TransportQueueElement.inl"
#endif /* !__ACE_INLINE__ */

OpenDDS::DCPS::TransportQueueElement::~TransportQueueElement()
{
  DBG_ENTRY_LVL("TransportQueueElement","~TransportQueueElement",6);
}

bool
OpenDDS::DCPS::TransportQueueElement::requires_exclusive_packet() const
{
  DBG_ENTRY_LVL("TransportQueueElement","requires_exclusive_packet",6);
  return false;
}

bool
OpenDDS::DCPS::TransportQueueElement::is_control(RepoId pub_id) const
{
  DBG_ENTRY_LVL("TransportQueueElement","is_control",6);
  ACE_UNUSED_ARG(pub_id);
  return false;
}


ACE_Message_Block*
OpenDDS::DCPS::TransportQueueElement::clone(const ACE_Message_Block* msg,
                                            MessageBlockAllocator* mb_allocator,
                                            DataBlockAllocator* db_allocator)
{
  DBG_ENTRY_LVL("TransportQueueElement","clone",6);
  ACE_Message_Block* cur_block = const_cast<ACE_Message_Block* > (msg);
  ACE_Message_Block* copy = 0;
  ACE_Message_Block* cur_copy = 0;
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
    cur_copy->copy (cur_block->base (), cur_block->size ());
    cur_copy->rd_ptr (cur_copy->base () + (cur_block->rd_ptr () - cur_block->base ()));
    cur_copy->wr_ptr (cur_copy->base () + (cur_block->wr_ptr () - cur_block->base ()));

    if (copy == 0) {
      copy = cur_copy;
    }
    else {
      copy->cont (cur_copy);
    }
    cur_block = cur_block->cont();
  }

  return copy;
}

