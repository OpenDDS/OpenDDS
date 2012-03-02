/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "TransportSendElement.h"
#include "TransportSendListener.h"

#if !defined (__ACE_INLINE__)
#include "TransportSendElement.inl"
#endif /* __ACE_INLINE__ */

OpenDDS::DCPS::TransportSendElement::~TransportSendElement()
{
  DBG_ENTRY_LVL("TransportSendElement","~TransportSendElement",6);
}

void
OpenDDS::DCPS::TransportSendElement::release_element(bool dropped_by_transport)
{
  DBG_ENTRY_LVL("TransportSendElement","release_element",6);

  if (this->was_dropped()) {
    this->element_->send_listener_->data_dropped(this->element_,
                                                 dropped_by_transport);

  } else {
    this->element_->send_listener_->data_delivered(this->element_);
  }

  if (allocator_) {
    ACE_DES_FREE(this, allocator_->free, TransportSendElement);
  }
}

OpenDDS::DCPS::RepoId
OpenDDS::DCPS::TransportSendElement::publication_id() const
{
  DBG_ENTRY_LVL("TransportSendElement","publication_id",6);
  return this->element_->publication_id_;
}

const ACE_Message_Block*
OpenDDS::DCPS::TransportSendElement::msg() const
{
  DBG_ENTRY_LVL("TransportSendElement","msg",6);
  return this->element_->sample_;
}

const ACE_Message_Block*
OpenDDS::DCPS::TransportSendElement::msg_payload() const
{
  DBG_ENTRY_LVL("TransportSendElement", "msg_payload", 6);
  return this->element_->sample_ ? this->element_->sample_->cont() : 0;
}

const OpenDDS::DCPS::DataSampleListElement*
OpenDDS::DCPS::TransportSendElement::sample() const
{
  DBG_ENTRY_LVL("TransportSendElement","sample",6);
  return this->element_;
}
