// -*- C++ -*-
//
// $Id$
#include  "DCPS/DdsDcps_pch.h"
#include  "TransportControlElement.h"
#include  "dds/DCPS/transport/framework/EntryExit.h"


TAO::DCPS::TransportControlElement::TransportControlElement(ACE_Message_Block* msg_block)
: TransportQueueElement(0),
  msg_ (msg_block)
{
  DBG_ENTRY("TransportControlElement","TransportControlElement");
}


TAO::DCPS::TransportControlElement::~TransportControlElement()
{
  DBG_ENTRY("TransportControlElement","~TransportControlElement");
}


bool
TAO::DCPS::TransportControlElement::requires_exclusive_packet() const
{
  DBG_ENTRY("TransportControlElement","requires_exclusive_packet");
  return true;
}


void
TAO::DCPS::TransportControlElement::release_element(bool dropped_by_transport)
{
  ACE_UNUSED_ARG (dropped_by_transport);
}


void
TAO::DCPS::TransportControlElement::data_delivered()
{
  DBG_ENTRY("TransportSendControlElement","data_delivered");
}


TAO::DCPS::RepoId 
TAO::DCPS::TransportControlElement::publication_id() const 
{
  return 0; 
}


const ACE_Message_Block* 
TAO::DCPS::TransportControlElement::msg() const 
{
  return this->msg_; 
}


