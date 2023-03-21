/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "TransportCustomizedElement.h"
#include "TransportSendListener.h"
#include "TransportSendElement.h"

#if !defined (__ACE_INLINE__)
#include "TransportCustomizedElement.inl"
#endif /* __ACE_INLINE__ */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

TransportCustomizedElement::~TransportCustomizedElement()
{
  DBG_ENTRY_LVL("TransportCustomizedElement", "~TransportCustomizedElement", 6);
}

void
TransportCustomizedElement::release_element(bool dropped_by_transport)
{
  DBG_ENTRY_LVL("TransportCustomizedElement", "release_element", 6);
  TransportQueueElement* decided = 0;
  if (orig_) {
    decided = orig_;
  }

  delete this;

  if (decided) {
    decided->decision_made(dropped_by_transport);
  }
}

ACE_Message_Block*
TransportCustomizedElement::duplicate_msg() const
{
  DBG_ENTRY_LVL("TransportCustomizedElement", "duplicate_msg", 6);
  return msg_->duplicate();
}

const ACE_Message_Block*
TransportCustomizedElement::msg() const
{
  DBG_ENTRY_LVL("TransportCustomizedElement", "msg", 6);
  return msg_.get();
}

void
TransportCustomizedElement::set_msg(Message_Block_Ptr m)
{
  DBG_ENTRY_LVL("TransportCustomizedElement", "set_msg", 6);
  msg_.reset(m.release());
}

const ACE_Message_Block*
TransportCustomizedElement::msg_payload() const
{
  DBG_ENTRY_LVL("TransportCustomizedElement", "msg_payload", 6);
  return orig_ ? orig_->msg_payload() : 0;
}

const TransportSendElement*
TransportCustomizedElement::find_original_send_element(TransportQueueElement* orig)
{
  const TransportSendElement* ose =
    dynamic_cast<const TransportSendElement*>(orig);
  if (!ose) {
    const TransportCustomizedElement* tce =
      dynamic_cast<const TransportCustomizedElement*>(orig);
    return tce ? tce->original_send_element() : 0;
  }
  return ose;
}

const TransportSendElement*
TransportCustomizedElement::original_send_element() const
{
  return original_send_element_;
}

bool
TransportCustomizedElement::is_last_fragment() const
{
  return original_send_element_ ? original_send_element_->is_last_fragment() : false;
}


} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
