// -*- C++ -*-
//
// $Id$
#include  "DCPS/DdsDcps_pch.h"
#include  "TransportReplacedElement.h"


#if !defined (__ACE_INLINE__)
#include "TransportReplacedElement.inl"
#endif /* __ACE_INLINE__ */


TAO::DCPS::TransportReplacedElement::~TransportReplacedElement()
{
  DBG_ENTRY_LVL("TransportReplacedElement","~TransportReplacedElement",5);
}


void
TAO::DCPS::TransportReplacedElement::release_element(bool dropped_by_transport)
{
  ACE_UNUSED_ARG (dropped_by_transport);
  DBG_ENTRY_LVL("TransportReplacedElement","release_element",5);

  if (this->msg_ != 0)
    {
      this->msg_->release();
      this->msg_ = 0;
    }

  if (allocator_)
    {
      allocator_->free (this);
    }
}


TAO::DCPS::RepoId
TAO::DCPS::TransportReplacedElement::publication_id() const
{
  DBG_ENTRY_LVL("TransportReplacedElement","publication_id",5);
  return this->publisher_id_;
}


const ACE_Message_Block*
TAO::DCPS::TransportReplacedElement::msg() const
{
  DBG_ENTRY_LVL("TransportReplacedElement","msg",5);
  return this->msg_;
}
