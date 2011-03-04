/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "TransportCustomizedElement.h"
#include "TransportSendListener.h"

#if !defined (__ACE_INLINE__)
#include "TransportCustomizedElement.inl"
#endif /* __ACE_INLINE__ */

namespace OpenDDS {
namespace DCPS {

TransportCustomizedElement::~TransportCustomizedElement()
{
  DBG_ENTRY_LVL("TransportCustomizedElement", "~TransportCustomizedElement", 6);
  if (msg_) {
    msg_->release();
  }
}

void
TransportCustomizedElement::release_element(bool dropped_by_transport)
{
  DBG_ENTRY_LVL("TransportCustomizedElement", "release_element", 6);
  tse_->decision_made(dropped_by_transport);

  if (allocator_) {
    ACE_DES_FREE(this, allocator_->free, TransportCustomizedElement);
  }
}

RepoId
TransportCustomizedElement::publication_id() const
{
  DBG_ENTRY_LVL("TransportCustomizedElement", "publication_id", 6);
  return tse_->publication_id();
}

const ACE_Message_Block*
TransportCustomizedElement::msg() const
{
  DBG_ENTRY_LVL("TransportCustomizedElement", "msg", 6);
  return msg_;
}

void
TransportCustomizedElement::set_msg(ACE_Message_Block* m)
{
  DBG_ENTRY_LVL("TransportCustomizedElement", "set_msg", 6);
  msg_ = m;
}

} // namespace DCPS
} // namespace OpenDDS
