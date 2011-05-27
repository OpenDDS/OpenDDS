// -*- C++ -*-
//
// $Id$
#include  "DCPS/DdsDcps_pch.h"
#include  "TransportSendElement.h"
#include  "dds/DCPS/DataSampleList.h"
#include  "TransportSendListener.h"

#if !defined (__ACE_INLINE__)
#include "TransportSendElement.inl"
#endif /* __ACE_INLINE__ */



TAO::DCPS::TransportSendElement::~TransportSendElement()
{
  DBG_ENTRY("TransportSendElement","~TransportSendElement");
}


void
TAO::DCPS::TransportSendElement::release_element()
{
  DBG_ENTRY("TransportSendElement","release_element");

  if (this->was_dropped())
    {
      this->element_->send_listener_->data_dropped(this->element_);
    }
  else
    {
      this->element_->send_listener_->data_delivered(this->element_);
    }
}


TAO::DCPS::RepoId
TAO::DCPS::TransportSendElement::publication_id() const
{
  DBG_ENTRY("TransportSendElement","publication_id");
  return this->element_->publication_id_;
}


const ACE_Message_Block*
TAO::DCPS::TransportSendElement::msg() const
{
  DBG_ENTRY("TransportSendElement","msg");
  return this->element_->sample_;
}

