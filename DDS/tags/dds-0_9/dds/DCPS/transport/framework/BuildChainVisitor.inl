// -*- C++ -*-
//
// $Id$

#include  "EntryExit.h"

ACE_INLINE
TAO::DCPS::BuildChainVisitor::BuildChainVisitor()
  : head_(0),
    tail_(0)
{
  DBG_ENTRY("BuildChainVisitor","BuildChainVisitor");
}



ACE_INLINE
ACE_Message_Block*
TAO::DCPS::BuildChainVisitor::chain()
{
  DBG_ENTRY("BuildChainVisitor","chain");

  ACE_Message_Block* head = this->head_;
  this->head_ = this->tail_ = 0;
  return head;
}

