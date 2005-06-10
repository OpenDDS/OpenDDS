// -*- C++ -*-
//
// $Id$
#include  "DCPS/DdsDcps_pch.h"
#include  "TransportSendControlElement.h"
#include  "TransportSendListener.h"
#include  "dds/DCPS/transport/framework/EntryExit.h"

#if !defined (__ACE_INLINE__)
#include "TransportSendControlElement.inl"
#endif /* __ACE_INLINE__ */


TAO::DCPS::TransportSendControlElement::~TransportSendControlElement()
{
  DBG_ENTRY("TransportSendControlElement","~TransportSendControlElement");
}


bool
TAO::DCPS::TransportSendControlElement::requires_exclusive_packet() const
{
  DBG_ENTRY("TransportSendControlElement","requires_exclusive_packet");
  return true;
}


void
TAO::DCPS::TransportSendControlElement::release_element()
{
  DBG_ENTRY("TransportSendControlElement","release_element");

  if (this->was_dropped())
    {
      this->listener_->control_dropped(this->msg_);
    }
  else
    {
      this->listener_->control_delivered(this->msg_);
    }
}


TAO::DCPS::RepoId
TAO::DCPS::TransportSendControlElement::publication_id() const
{
  DBG_ENTRY("TransportSendControlElement","publication_id");
  return this->publisher_id_;
}


const ACE_Message_Block*
TAO::DCPS::TransportSendControlElement::msg() const
{
  DBG_ENTRY("TransportSendControlElement","msg");
  return this->msg_;
}


bool
TAO::DCPS::TransportSendControlElement::is_control(RepoId pub_id) const
{
  return (pub_id == this->publisher_id_);
}

