/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "TransportControlElement.h"
#include "dds/DCPS/transport/framework/EntryExit.h"

OpenDDS::DCPS::TransportControlElement::TransportControlElement(
  const ACE_Message_Block* msg_block,
  const RepoId& pub_id,
  bool owner
) : TransportQueueElement(1),
    msg_( ACE_Message_Block::duplicate(msg_block)),
    pub_id_( pub_id),
    owner_( owner)
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
OpenDDS::DCPS::TransportControlElement::release_element(
  bool /* dropped_by_transport */
)
{
  if (this->msg_) {
    this->msg_->release();
    this->msg_ = 0;
  }

  if (this->owner_) {
    delete this;
  }
}

void
OpenDDS::DCPS::TransportControlElement::data_delivered()
{
  DBG_ENTRY_LVL("TransportSendControlElement","data_delivered",6);
}

OpenDDS::DCPS::RepoId
OpenDDS::DCPS::TransportControlElement::publication_id() const
{
  return GUID_UNKNOWN;
}

const ACE_Message_Block*
OpenDDS::DCPS::TransportControlElement::msg() const
{
  return this->msg_;
}

ACE_INLINE
bool 
OpenDDS::DCPS::TransportControlElement::owned_by_transport ()
{
  return true;
}

