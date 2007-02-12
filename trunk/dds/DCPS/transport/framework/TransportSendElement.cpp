// -*- C++ -*-
//
// $Id$
#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "TransportSendElement.h"
#include "TransportSendListener.h"

#if !defined (__ACE_INLINE__)
#include "TransportSendElement.inl"
#endif /* __ACE_INLINE__ */



TAO::DCPS::TransportSendElement::~TransportSendElement()
{
  DBG_ENTRY_LVL("TransportSendElement","~TransportSendElement",5);
}


void
TAO::DCPS::TransportSendElement::release_element(bool dropped_by_transport)
{
  DBG_ENTRY_LVL("TransportSendElement","release_element",5);

  if (this->was_dropped())
    {
      this->element_->send_listener_->data_dropped(this->element_, dropped_by_transport);
    }
  else
    {
      this->element_->send_listener_->data_delivered(this->element_);
    }

  if (allocator_)
    {
      allocator_->free (this);
    }
}


TAO::DCPS::RepoId
TAO::DCPS::TransportSendElement::publication_id() const
{
  DBG_ENTRY_LVL("TransportSendElement","publication_id",5);
  return this->element_->publication_id_;
}


const ACE_Message_Block*
TAO::DCPS::TransportSendElement::msg() const
{
  DBG_ENTRY_LVL("TransportSendElement","msg",5);
  return this->element_->sample_;
}

