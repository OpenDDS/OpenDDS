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

namespace
{
  void handle_message(const bool dropped,
                      ACE_Message_Block* const msg,
                      OpenDDS::DCPS::TransportSendListener* const listener,
                      const bool dropped_by_transport)
  {
    if (dropped) {
      listener->control_dropped(msg, dropped_by_transport);
    } else {
      listener->control_delivered(msg);
    }
  }
}

void
OpenDDS::DCPS::TransportSendControlElement::release_element(bool dropped_by_transport)
{
  ACE_UNUSED_ARG(dropped_by_transport);

  DBG_ENTRY_LVL("TransportSendControlElement","release_element",6);

  // store off local copies to use after "this" pointer deleted
  const bool dropped = this->was_dropped();
  ACE_Message_Block* const msg = this->msg_;
  TransportSendListener* const listener = this->listener_;

  if (allocator_) {
    ACE_DES_FREE(this, allocator_->free, TransportSendControlElement);
  }

  // reporting the message w/o using "this" pointer
  handle_message(dropped, msg, listener, dropped_by_transport);
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

const ACE_Message_Block*
OpenDDS::DCPS::TransportSendControlElement::msg_payload() const
{
  DBG_ENTRY_LVL("TransportSendControlElement", "msg_payload", 6);
  return this->msg_->cont();
}

bool
OpenDDS::DCPS::TransportSendControlElement::is_control(RepoId pub_id) const
{
  return (pub_id == this->publisher_id_);
}
