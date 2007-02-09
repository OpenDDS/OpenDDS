// -*- C++ -*-
//
// $Id$

#include  "DCPS/DdsDcps_pch.h"
#include  "BuildChainVisitor.h"
#include  "TransportQueueElement.h"

#if !defined (__ACE_INLINE__)
#include "BuildChainVisitor.inl"
#endif /* __ACE_INLINE__ */

TAO::DCPS::BuildChainVisitor::~BuildChainVisitor()
{
  DBG_ENTRY_LVL("BuildChainVisitor","~BuildChainVisitor",5);
}


int
TAO::DCPS::BuildChainVisitor::visit_element(TransportQueueElement* element)
{
  DBG_ENTRY_LVL("BuildChainVisitor","visit_element",5);

  if (this->head_ == 0)
    {
      // This is the first element that we have visited.
      this->head_ = element->msg()->duplicate();
      this->tail_ = this->head_;
      while (this->tail_->cont() != 0)
        {
          this->tail_ = this->tail_->cont();
        }
    }
  else
    {
      // This is not the first element that we have visited.
      this->tail_->cont(element->msg()->duplicate());
      while (this->tail_->cont() != 0)
        {
          this->tail_ = this->tail_->cont();
        }
    }

  // Always continue visitation.
  return 1;
//MJM: I assume here that you limit packet sizes external to this.
//MJM: Hmm... I guess that I don't understand the context of this
//MJM: visitor yet.
}

