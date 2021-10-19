/*
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

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

OpenDDS::DCPS::TransportSendElement::~TransportSendElement()
{
  DBG_ENTRY_LVL("TransportSendElement", "~TransportSendElement", 6);
}

void
OpenDDS::DCPS::TransportSendElement::release_element(bool dropped_by_transport)
{
  DBG_ENTRY_LVL("TransportSendElement", "release_element", 6);

  if (element_->get_send_listener()) {
    if (was_dropped()) {
      element_->get_send_listener()->data_dropped(element_, dropped_by_transport);
    } else {
      element_->get_send_listener()->data_delivered(element_);
    }
  }

  delete this;
}

OpenDDS::DCPS::RepoId
OpenDDS::DCPS::TransportSendElement::publication_id() const
{
  DBG_ENTRY_LVL("TransportSendElement", "publication_id", 6);
  return element_->get_pub_id();
}

OpenDDS::DCPS::RepoId
OpenDDS::DCPS::TransportSendElement::subscription_id() const
{
  if (element_->get_num_subs() == 1) {
    return element_->get_sub_id(0);
  }
  return GUID_UNKNOWN;
}

ACE_Message_Block*
OpenDDS::DCPS::TransportSendElement::duplicate_msg() const
{
  DBG_ENTRY_LVL("TransportSendElement", "duplicate_msg", 6);
  return element_->get_sample()->duplicate();
}

const ACE_Message_Block*
OpenDDS::DCPS::TransportSendElement::msg() const
{
  DBG_ENTRY_LVL("TransportSendElement", "msg", 6);
  return element_->get_sample();
}

const ACE_Message_Block*
OpenDDS::DCPS::TransportSendElement::msg_payload() const
{
  DBG_ENTRY_LVL("TransportSendElement", "msg_payload", 6);
  return element_->get_sample() ? element_->get_sample()->cont() : 0;
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
