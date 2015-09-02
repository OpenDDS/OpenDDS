/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "TransportControlElement.h"
#include "dds/DCPS/transport/framework/EntryExit.h"

#if !defined (__ACE_INLINE__)
#include "TransportControlElement.inl"
#endif /* __ACE_INLINE__ */

OpenDDS::DCPS::TransportControlElement::TransportControlElement(
  const ACE_Message_Block* msg_block,
  const RepoId& /*pub_id*/,
  bool owner
) : TransportQueueElement(1),
    msg_( ACE_Message_Block::duplicate(msg_block)),
    owner_( owner)
{
  DBG_ENTRY_LVL("TransportControlElement","TransportControlElement",6);
}

OpenDDS::DCPS::TransportControlElement::~TransportControlElement()
{
  DBG_ENTRY_LVL("TransportControlElement","~TransportControlElement",6);
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
