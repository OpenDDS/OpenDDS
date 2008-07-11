// -*- C++ -*-
//
// $Id$
#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "TransportControlElement.h"
#include "dds/DCPS/transport/framework/EntryExit.h"


OpenDDS::DCPS::TransportControlElement::TransportControlElement(ACE_Message_Block* msg_block)
: TransportQueueElement(1),
  msg_ (msg_block)
{
  DBG_ENTRY_LVL("TransportControlElement","TransportControlElement",6);
}


OpenDDS::DCPS::TransportControlElement::~TransportControlElement()
{
  DBG_ENTRY_LVL("TransportControlElement","~TransportControlElement",6);
}


bool
OpenDDS::DCPS::TransportControlElement::requires_exclusive_packet() const
{
  DBG_ENTRY_LVL("TransportControlElement","requires_exclusive_packet",6);
  return true;
}


void
OpenDDS::DCPS::TransportControlElement::release_element(bool dropped_by_transport)
{
  ACE_UNUSED_ARG (dropped_by_transport);

  if (msg_) {
    msg_->release ();
  }

  // This element is guaranteed to be heap-based. The DCPS layer passes
  // a ptr to this object to the transport layer, hense the guarantee.
  delete this;
}


void
OpenDDS::DCPS::TransportControlElement::data_delivered()
{
  DBG_ENTRY_LVL("TransportSendControlElement","data_delivered",6);
}


OpenDDS::DCPS::RepoId
OpenDDS::DCPS::TransportControlElement::publication_id() const
{
  return 0;
}


const ACE_Message_Block*
OpenDDS::DCPS::TransportControlElement::msg() const
{
  return this->msg_;
}
