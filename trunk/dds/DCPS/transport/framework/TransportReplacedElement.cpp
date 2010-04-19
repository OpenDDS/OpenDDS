/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "TransportReplacedElement.h"

#if !defined (__ACE_INLINE__)
#include "TransportReplacedElement.inl"
#endif /* __ACE_INLINE__ */

OpenDDS::DCPS::TransportReplacedElement::~TransportReplacedElement()
{
  DBG_ENTRY_LVL("TransportReplacedElement","~TransportReplacedElement",6);
}

void
OpenDDS::DCPS::TransportReplacedElement::release_element(bool dropped_by_transport)
{
  ACE_UNUSED_ARG(dropped_by_transport);
  DBG_ENTRY_LVL("TransportReplacedElement","release_element",6);

  if (this->msg_ != 0) {
    this->msg_->release();
    this->msg_ = 0;
  }

  if (allocator_) {
    allocator_->free(this);
  }
}

OpenDDS::DCPS::RepoId
OpenDDS::DCPS::TransportReplacedElement::publication_id() const
{
  DBG_ENTRY_LVL("TransportReplacedElement","publication_id",6);
  return this->publisher_id_;
}

const ACE_Message_Block*
OpenDDS::DCPS::TransportReplacedElement::msg() const
{
  DBG_ENTRY_LVL("TransportReplacedElement","msg",6);
  return this->msg_;
}


ACE_Message_Block*
OpenDDS::DCPS::TransportReplacedElement::clone(const ACE_Message_Block* msg)
{
  DBG_ENTRY_LVL("TransportReplacedElement","msg",6);
  ACE_Message_Block* cur_block = const_cast<ACE_Message_Block* > (msg);
  ACE_Message_Block* copy = 0;
  ACE_Message_Block* cur_copy = 0;
  // deep copy sample data
  while (cur_block != 0) {
    ACE_NEW_MALLOC_RETURN(cur_copy,
                          static_cast<ACE_Message_Block*>(
                          this->mb_allocator_->malloc(sizeof(ACE_Message_Block))),
                          ACE_Message_Block(cur_block->size (),
                                            ACE_Message_Block::MB_DATA,
                                            0, //cont
                                            0, //data
                                            0, //alloc_strategy
                                            0, //locking_strategy
                                            ACE_DEFAULT_MESSAGE_BLOCK_PRIORITY,
                                            ACE_Time_Value::zero,
                                            ACE_Time_Value::max_time,
                                            this->db_allocator_,
                                            this->mb_allocator_),
                          0);
    cur_copy->copy (cur_block->base (), cur_block->size ());
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