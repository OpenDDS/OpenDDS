// -*- C++ -*-
//
// $Id$
#include  "DCPS/DdsDcps_pch.h"
#include  "TransportGDControlElement.h"
#include  "dds/DCPS/transport/framework/EntryExit.h"


TAO::DCPS::TransportGDControlElement::TransportGDControlElement(ACE_Message_Block* msg_block)
: TransportQueueElement(0),
  msg_ (msg_block)
{
  DBG_ENTRY("TransportGDControlElement","TransportGDControlElement");
}


TAO::DCPS::TransportGDControlElement::~TransportGDControlElement()
{
  DBG_ENTRY("TransportGDControlElement","~TransportGDControlElement");
}


bool
TAO::DCPS::TransportGDControlElement::requires_exclusive_packet() const
{
  DBG_ENTRY("TransportGDControlElement","requires_exclusive_packet");
  return true;
}


void
TAO::DCPS::TransportGDControlElement::release_element(bool dropped_by_transport)
{
  ACE_UNUSED_ARG (dropped_by_transport);
}


void
TAO::DCPS::TransportGDControlElement::data_delivered()
{
  DBG_ENTRY("TransportSendControlElement","data_delivered");
}


TAO::DCPS::RepoId 
TAO::DCPS::TransportGDControlElement::publication_id() const 
{
  return 0; 
}


const ACE_Message_Block* 
TAO::DCPS::TransportGDControlElement::msg() const 
{
  return this->msg_; 
}


