/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "TransportSendControlElement.h"
#include "TransportSendListener.h"
#include "dds/DCPS/transport/framework/EntryExit.h"

#if !defined (__ACE_INLINE__)
#include "TransportSendControlElement.inl"
#endif /* __ACE_INLINE__ */

OpenDDS::DCPS::TransportSendControlElement::~TransportSendControlElement()
{
  DBG_ENTRY_LVL("TransportSendControlElement","~TransportSendControlElement",6);
}

bool
OpenDDS::DCPS::TransportSendControlElement::requires_exclusive_packet() const
{
  DBG_ENTRY_LVL("TransportSendControlElement","requires_exclusive_packet",6);
  return true;
}

void
OpenDDS::DCPS::TransportSendControlElement::release_element(bool dropped_by_transport)
{
  ACE_UNUSED_ARG(dropped_by_transport);

  DBG_ENTRY_LVL("TransportSendControlElement","release_element",6);

  if (this->was_dropped()) {
    this->listener_->control_dropped(this->msg_, dropped_by_transport);

  } else {
    this->listener_->control_delivered(this->msg_);
  }

  if (allocator_) {
    ACE_DES_FREE(this, allocator_->free, TransportSendControlElement);
  }
}

OpenDDS::DCPS::RepoId
OpenDDS::DCPS::TransportSendControlElement::publication_id() const
{
  DBG_ENTRY_LVL("TransportSendControlElement","publication_id",6);
  return this->publisher_id_;
}

const ACE_Message_Block*
OpenDDS::DCPS::TransportSendControlElement::msg() const
{
  DBG_ENTRY_LVL("TransportSendControlElement","msg",6);
  return this->msg_;
}

bool
OpenDDS::DCPS::TransportSendControlElement::is_control(RepoId pub_id) const
{
  return (pub_id == this->publisher_id_);
}
