// -*- C++ -*-
//
// $Id$

#include  "EntryExit.h"

ACE_INLINE
TAO::DCPS::BuildChainVisitor::BuildChainVisitor()
  : head_(0),
    tail_(0)
{
  DBG_ENTRY_LVL("BuildChainVisitor","BuildChainVisitor",5);
}



ACE_INLINE
ACE_Message_Block*
TAO::DCPS::BuildChainVisitor::chain()
{
  DBG_ENTRY_LVL("BuildChainVisitor","chain",5);

  ACE_Message_Block* head = this->head_;
  this->head_ = this->tail_ = 0;
  return head;
}

